# TODO

This document tracks what is planned for Sparkle. Active work is grouped by the
kind of change it is and ordered roughly correctness-first: the sections that
keep the interpreter from crashing or misreporting come before the ones that add
features, then speed, then polish. `Done` at the end is the record of what has
already landed, with the verification notes taken at the time.

## Correctness & Safety

The interpreter can be driven to crash or to invoke undefined behaviour by
ordinary programs. These come before features: a language that segfaults out of
`=` is one a program cannot trust.

* [ ] Deep recursion overflows the C stack and kills the process instead of
      raising something catchable. Each Sparkle-level call costs several C frames
      (`vm_eval_object` -> `vm_eval_list` -> `vm_call` -> `vm_call_lambda` ->
      `vm_eval_object`); `spk_eq` recurses on its own path through nested lists
      too, so a ceiling has to cover both. Wants tail-call elimination, or a
      depth limit that raises, or an explicit control stack. A depth limit would
      also make the last cyclic-structure
      crash catchable: `=` now short-circuits on identity, so `(= x x)` on a
      self-referential list terminates, but two distinct mutually-cyclic lists
      still overflow the stack - Python raises `RecursionError` there, so this is
      low priority, just a segfault where Python does not.
      *(verified: depth 10000 fine, 50000 overflows under `make debug`; two
      mutually-cyclic lists compared with `=` overflow it too)*

* [ ] The parser's recursion is a third crash surface, separate from the
      evaluator's. `parser_read_expr` recurses per nesting level, so a source
      file that is nothing but open parens kills the process before the VM ever
      runs - and unlike the evaluator, the fix is trivial and independent: a
      depth counter in the parser that sets `is_err` past a limit, no control
      stack needed. *(verified: 100k nested parens overflow the C stack under
      `make debug`)*

* [ ] Nothing bounds interpreter memory. A runaway program takes the machine
      down with it instead of raising.

* [ ] Allocation failure is unchecked in release. `da_init` and `da_push` guard
      `malloc` and `realloc` with `assert`, which `NDEBUG` compiles out, so the
      release build writes through a null pointer under memory pressure rather
      than failing. The GC constructors inherit this. The fix is to split two
      things that are both `assert` today: an invariant the code guarantees (fine
      to compile out) from a runtime condition the environment imposes (must be
      checked always). *(verified: `dynamic_array.h:22` and `:31` are asserts;
      release defines `NDEBUG`)*

* [ ] The builtin stack protocol is kept by discipline, not checked by the
      machine. Every builtin hand-balances the value stack and states its effect
      only in a comment (`String, Integer -> String`); nothing verifies it
      consumed its arguments and left exactly one result. A stray `vm_pop` on a
      cold branch is silent stack corruption the suite catches only if ASan trips
      on the fallout later. The check is cheap and belongs at the one dispatch
      site: `vm_call_builtin` knows the input count before it calls
      (`vm_eval.c:86` - `argc` slots for a fixed builtin, `arity + 1` for a
      variadic one), so record `value_stack.size` there and, on normal return,
      assert it fell to exactly that minus one. Debug builds only; an unwinding
      `vm_recover` skips the check by construction, which is correct - the
      recovery frame has already truncated the stack.

## Error Reporting

Two pieces landed, and no more are planned: parser errors carry a position, and
a runtime error can carry a message a program catches and the reporter prints.

* [x] **Parser errors carry their position and name what was found.** The
      parser records the first failure - a `TextPos`, the offending token's text,
      and a static message - and `io_report_parser` prints `line:column` the way
      the lexer already does, or "at end of input" when the tokens ran out. The
      first cause wins, so a bad escape inside a list is not masked by the `)`
      the frame then misses. *(verified: a stray `)` reports its line:col, `(1 2`
      reports at end of input, a bad `\x` escape points at the string)*

* [x] **Runtime errors can carry a message.** `vm_recover_msg` (with
      `VM_RECOVER_IF_MSG` beside `VM_RECOVER_IF`) raises the kind carrying a
      literal, built as an `Exception` whose value is a `String` borrowing the
      literal - no copy of the text, and `gc_alloc_*` never collect on the
      unwind. A handler reads it with `exc-value`; uncaught, it reports as
      `Kind: message`. The special-form shape errors (`let`, `set`, `while`,
      `quote`) carry their own line; the rest move over one at a time,
      `VM_RECOVER_IF` to `VM_RECOVER_IF_MSG`.

## Language Semantics

Decisions the language still owes, each to be made and then written into
`Specification.md`.

* [ ] Builtins may be shadowed, like Python: a later binding of a builtin's name
      takes precedence in its scope. Decided, and true in any nested scope - but
      not at the top level, because `register_builtins` writes into the same root
      scope top-level code binds in, so `(let + 5)` in a file is a same-scope
      collision and raises `REBINDING_EXCEPTION`. Python avoids this by giving
      builtins their own scope outside the module's; the fix here is the same
      shape: push a second scope in `interp_alloc` after registration, so the
      root the program sees sits above the builtins rather than beside them.
      Then say so in `Specification.md`. *(verified: `(begin (let + 5) ...)`
      shadows; top-level `(let + 5)` raises `REBINDING_EXCEPTION`)*

* [ ] A binding whose name starts with a capital is unreachable, with no
      diagnostic. `let` and `set` take any `Symbol`, but a capitalized symbol
      self-evaluates before the scope is consulted (`vm_eval_symbol`,
      `vm_eval.c:233`), so `(let Foo 1)` binds a name that evaluating `Foo` can
      never read back - it yields the symbol `Foo`. The rule that ties
      capitalization to self-evaluation is what buys symbols-as-enums, but it
      also makes a whole class of bindings write-only and silent. Decide: raise
      `VALUE_EXCEPTION` when `let`/`set` is handed a self-evaluating name
      (recommended - such a binding is never useful), or allow it and say so in
      `Specification.md`. *(verified: `(let Foo 1) Foo` prints `Foo`, not `1`)*

* [ ] `(- 5)` is `5`, not `-5`. The fold rule is uniform - one argument folds to
      itself, exactly as `(+ 5)` does - but negation is what every reader of a
      Lisp expects from unary minus, and `neg` existing does not stop the
      surprise. Scheme special-cases arity one for `-` and `/` for this reason.
      Decide: keep the uniform fold and say so loudly under Arithmetic, or
      special-case `(- x)` as negation. *(verified: `(- 5)` prints `5`)*

* [ ] Integer overflow is unspecified, and the reader has it too. `Integer` is a
      C `long long` and signed overflow is undefined behaviour - in arithmetic,
      where `(* 99999999999 99999999999)` is whatever the optimizer decides, and
      already in `svtolli`, where a literal too big for the type overflows while
      being read, so a program consisting of one long number executes UB before
      anything runs. Pick wrapping, saturation, promotion or an exception, apply
      it to literals as well as operators, then say so in `Specification.md`.
      *(verified under UBSan: `string_view.c:131` on the literal,
      `builtins_arithmetic.c:35` on the multiply; release prints garbage)*

* [ ] Float printing does not round-trip. The form is now specified - fixed
      point, six decimals, never an exponent - so the loss is at least written
      down rather than merely suffered, but a magnitude below `0.0000005` still
      prints as `0.000000` and `inf` and `nan` do not read back at all. Wants a
      shortest-round-trip form, and literals the reader accepts for the
      non-finite values.

* [ ] A `Builtin` has no printed form. `write_expr` writes nothing for
      `KIND_BUILTIN`, so `(str +)` is `""` and `(print "$0" +)` prints an empty
      slot - while the spec promises `str` renders any type. Decide a form
      (other Lisps print `#<builtin +>`; even a bare `<builtin>` beats
      vanishing), spec it, and give `write_expr` the case. The name is not
      currently in reach of the object - either store the registration name in
      `BuiltinObject` or settle for a nameless form.

* [ ] Special-form names are not values, and the spec never says so. `if` in a
      non-head position is just an undefined symbol - `(let f if)` raises
      `UNDEFINED_EXCEPTION` - which follows from the evaluation model (the
      special-form check reads the head symbol before evaluation) but surprises
      anyone arriving from a language where everything is first-class. One
      sentence under Special Forms closes it. *(verified)*

* [ ] `String` is ASCII, and the ceiling only shows at the edges. The type is a
      byte sequence, `str-chr` rejects anything past 127, and
      `str-len`/`str-get`/`str-upper`/`str-split` all count and cut in bytes - so
      the ASCII limit stated under Types is real but invisible until a multibyte
      input misbehaves partway through. Decide whether it stays ASCII or grows a
      byte-versus-character distinction; either way the byte-orientation is worth
      stating where the operations are, not only on the type.

## Control Flow & Special Forms

* [ ] Early exit: no `break`, `continue` or `return`, so a loop cannot stop
      before its condition flips and a function cannot return from a branch.

* [ ] `cond`-style dispatch on a value rather than on a chain of predicates.

* [ ] No unwind protection: an exception unwinds past whatever cleanup the
      program wanted to run. Once file handles exist this stops being academic.

## Base Library

* [ ] `reduce` / `fold`.
* [ ] `sort` with a comparator - `tests/positive` hand-rolls merge and
      insertion sort because there is none.
* [ ] A keyed collection: there is no map, dictionary or set, so an association
      list walked by hand is the only lookup structure.
* [ ] File I/O. Standard input and output are the only streams.
* [ ] Access to command-line arguments and environment variables.
* [ ] Exit with a chosen status code.
* [ ] Clock / time.
* [ ] Random numbers.
* [ ] Formatting to a String. `print` understands `$N` placeholders but writes
      to stdout; there is no way to build the same string in memory.
* [ ] List basics that are missing and get hand-rolled instead: `reverse`,
      `range`, `slice`, `find`, `any` / `all`, `zip`.
* [ ] Math surface stops at `sqrt` and `pow`: no trigonometry, no logarithms,
      no `Pi` or `E`.
* [ ] Number parsing and printing in a chosen radix. The lexer reads `0b`,
      `0o` and `0x` literals, but a program cannot produce or consume them.

## Performance

Everything here is correct but scans where it could index. Measure before
committing to any of it.

* [ ] Integer Interning.
* [ ] Arena allocations instead of a linked list in GC.
* [ ] Scope lookup is a linear scan: `scope_get` walks every binding of every
      enclosing scope on each symbol evaluation. Index it.
* [ ] String interning is a linear scan over every string interned so far
      (`si_getn`), so interning is O(n) in the program's symbol count. Hash it.
* [ ] Special-form dispatch scans the whole table on every list evaluation
      (`vm_try_special_form`). The scan is now over `vm->special_forms`, so it is
      per-VM data rather than a global table.
* [ ] The collection threshold only ratchets up. `gc_grow_if_needed` doubles
      `capacity` every time the heap reaches it and nothing ever lowers it, so
      one allocation spike makes every later collection rarer and the high-water
      memory permanent. After a sweep, recompute the threshold from what
      survived (say twice the live count, floored at the initial capacity).

## Internals & Refactoring

Cleanups with no user-visible change.

* [ ] `X_LANGUAGE_SYMBOLS` still lives in `vm.h`, so the VM core spells out
      words only the language gives meaning to (`Nil`, `True`, `False`,
      `quote`, `Var`). It belongs in `lang/`, registered into the VM the way
      special forms now are - `vm_register_special_form` already shows the shape.
      The self-evaluating-symbol rule in `vm_eval_symbol` (capitalized symbols
      evaluate to themselves) has to move with it, since that is the only thing
      reading `Nil`/`True`/`False`. Moving it out of the interner was the first
      half of this.

* [ ] Type checking past two arguments is hand-rolled. `vm_expect`/`vm_expect2`
      stop at arity two, so `str-sub`, `str-replace` and `put` each open with
      three separate `VM_RECOVER_IF(..., TYPE_EXCEPTION)` lines restating what
      `vm_expect` exists to say. A `vm_expect_at(vm, depth, type)` or a variadic
      `vm_expect_n` routes every type guard through one place.

* [ ] The `Numeric -> double` widening is written twice. `cast_numeric`
      (`builtins_arithmetic.c:117`) and `numeric_as_double` (`:147`) carry the
      same `Bool`/`Integer`/`Float` switch with the same `UNREACHABLE` tail, so a
      new numeric kind is two edits. Fold to one helper.

## Syntax & Parser

* [ ] The lexer splits what the grammar calls one symbol. `Specification.md`
      defines `symbol = symbol-char+ - (integer | float)`, which makes `1ab` a
      symbol - all symbol characters, not a number. The lexer instead scans the
      longest number prefix and stops, so `1ab` lexes as `1` then `ab`, and
      `(print "$0" 1ab)` prints `1` where the spec demands
      `UNDEFINED_EXCEPTION`. Either the scanner refuses the number when a
      symbol character follows it (maximal munch, matching the written
      grammar), or the grammar documents the split. The first is truer to the
      one-document-defines-the-language rule. *(verified: `1ab` evaluates as
      two expressions)*

* [ ] Escape syntax for symbols that contain delimiters.

## Tooling

* [ ] No module system: a program is a single file, with no `import` or `load`,
      so nothing can be split up or reused across programs. When file loading
      arrives, `main.c`'s `read_file` errors - the last raw `fprintf(stderr)`
      calls outside `io.c` - move behind the Io component with it.
* [ ] Benchmark suite - the test runner times each case, but nothing tracks
      whether the interpreter is getting faster or slower.
* [ ] The tester's oracle is coarser than the output it checks. `format_output`
      splits on all whitespace, so a test cannot distinguish one line from two,
      a trailing space from none, or a blank line from nothing - and `print` is
      the language's whole output story, so its exact spacing is part of the
      contract. Positive tests also never look at stderr, so stray diagnostics
      pass unseen. Compare line-by-line with a normalized-whitespace fallback,
      and assert stderr is empty where no `.err` exists.
* [ ] CI never runs `make format`, so formatting drift lands silently. A check
      step - `clang-format --dry-run --Werror` over the sources - keeps the
      format target honest the way `-Werror` keeps the build.
* [ ] Editor support: syntax highlighting, at least a `.spk` grammar.
* [ ] No install target - the binary only exists at `./build/sparkle`.
* [ ] A trace mode that prints evaluation steps. The interpreter is a teaching
      artifact as much as a tool, and it cannot show its own work.

## CLI & REPL

* [ ] Proper interpreter interaction.
* [ ] The REPL is not covered by a single test. `tester.py` always invokes
      `[binary, spk_path]`, and the interactive mode is what runs when there is
      no path, so the suite cannot reach it. Wants a `tests/repl/` folder whose
      cases are fed to the binary on stdin with no argument; the comparison,
      the `.out` files and `--rewrite` all carry over unchanged.
* [ ] A REPL session exits 0 even when lines in it failed. Right for an
      interactive session, arguable for a piped one. Decided rather than
      stumbled into, but worth revisiting alongside exit statuses generally.
* [ ] REPL echo has no readable form for values. `write_expr` renders a String
      as its raw bytes, which is what `print` and `str` need, so `"hello"`
      echoes as `hello` and cannot be told from the symbol `hello` - and a
      string holding parens or spaces reads as structure. This is the `display`
      vs `write` split every Lisp ends up making: one form for showing a value
      to a user, one that is unambiguous and reads back.
      Wants a readable mode on `write_expr` used by echo alone, leaving `print`
      and `str` exactly as they are. The comment in `io.h` claiming one
      representation for everything has to become two, with the reason.
      *(verified: `(str 3.5)` and `"3.500000"` echo identically)*
* [ ] Run a program from standard input, and evaluate an expression given on the
      command line (`-e`).
* [ ] The CLI takes one argument and nothing else: no `--help`, no `--version`,
      and the usage line is the only thing resembling documentation.
* [ ] Exit statuses are undocumented. Today everything that fails exits 1, so a
      script cannot tell a syntax error from a runtime one.
* [ ] Report color is a policy flag nobody sets yet. `Io` carries `color` and
      `io_report` honors it, but `io_alloc_std` hardwires `true`, so a piped or
      logged session still gets raw escape bytes (the tester strips them on its
      side). What remains is the edge decision in `main`: `isatty` is POSIX, so
      either take it where available or add a `--no-color` flag; the plumbing
      no longer cares.

## Documentation & Presentation

* [ ] `Readme.md` does not open with what the language looks like. A reader
      deciding whether to keep scrolling wants a program in the first screen.
* [ ] A tutorial. `Specification.md` defines the language, which is not the
      same as teaching it: there is no path from zero to a working program
      short of reading the whole thing. This is what `README.md` should carry,
      now that it is the only document written for a reader rather than for an
      implementer.
* [ ] `examples/` holds two programs worth reading - `life.spk` and `rpn.spk`
      earn their place - but two is still a thin shelf for a language arguing it
      is pleasant to write. Breadth is what is missing now, not quality: a
      program per style the README claims (something recursive, something
      string-heavy, something that leans on `map`/`filter`).
* [ ] The README's warning banner overstates the danger. It promises "bugs,
      memory leaks, and undefined behavior", while CI runs the whole suite under
      ASan with `detect_leaks=1` and passes - the project is leak-checked on
      every push and the banner does not know it. Replace the self-deprecation
      with a short known-limitations note that points at this file; a portfolio
      reader believes warnings.
* [ ] `.gitignore` ignores itself, so it is not in the repository. A fresh clone
      gets no ignore rules at all: `build/` and editor droppings land in
      `git status` for anyone else touching the repo. Drop the `.gitignore`
      line from itself and commit the file.
* [ ] `README.md` promises more familiarity than the language keeps. It says
      Sparkle "works the way you would expect coming from Python or Lua," while
      several behaviours are deliberately not that: `set` assigns in parallel, so
      `(set x y y x)` swaps; capitalized symbols self-evaluate; there is no
      `return`, `break` or `continue`. Each is a defensible choice - but the
      sentence should name them as where Sparkle diverges, or narrow the claim,
      rather than let a reader meet them as surprises.
* [ ] Nothing keeps `Specification.md` honest as the language moves. A
      checklist in the test suite, or a test that reads the document. The
      merge that removed `Sparkle.md` found the gap the hard way: it was
      missing all thirteen string functions, claimed `(+)` with no arguments
      returns `0` when it raises `ARITY_EXCEPTION`, and carried a worked
      example of a multi-expression `lambda` body that did not parse until the
      body became a sequence.
* [ ] No version number and no changelog, so there is nothing to point at when
      something changes under a user.
* [ ] The reasoning in this file is really a design log. The `Done` notes and
      their verified observations, and the rationale threaded through the
      sections above are decisions with their why
      attached - worth more, and harder to find, than a task list should make
      them. A `DESIGN.md` (or `DECISIONS.md`) could hold the rationale and leave
      this file the tasks that point at it. Optional, and an editorial call about
      how the project wants to present itself.

## Done

* [x] IO became a component the VM is handed. `io.{c,h}` at the src/ root owns
      the three streams, the err-after-out flush ordering the REPL prompt and
      the diagnostics each used to spell by hand, severity and color, and the
      `io_report_*` formatters that absorbed `diagnostics.{c,h}` whole. The VM
      carries an `Io *` for the byte floor alone - `print`, `input` and echo
      move bytes through it - while reporting stays the shell's business after
      a run. `lang/io.{c,h}` became `lang/write.{c,h}`, putting `write_expr`
      under the file named for it and freeing the io name; `CharDA` moved to
      `dynamic_array.h`, typedef'd once; the read-line loop `repl_read_line`
      and `spk_input` each spelled collapsed into `io_read_line`. Color is a
      host-policy flag, hardwired on for now.
* [x] Lists compare structurally. `=` on two `List`s is true when they have the
      same length and equal elements, each compared by `=` in turn, so nested
      lists and the numeric coercion of elements come for free. `Lambda` and
      `Builtin` keep reference equality. `spk_eq` recurses on itself with the two
      lists held on the value stack so their elements stay rooted; a list made to
      contain itself has no terminating comparison and is not guarded against.
* [x] `try` catches any of several kinds. One kind is written bare,
      `(try KIND expr...)`; several are a list, `(try (K1 K2 ...) expr...)`, and
      any of them is caught - so the shape needs no marker, the same reason
      `for` reads its target by shape. Each kind is evaluated and packed into a
      `List` the handler scans on unwind, returning the raised kind on a match
      and re-raising otherwise; the four-deep tower of `try`s in
      `examples/rpn.spk` collapses to one. The `In` marker the original plan
      wanted was already gone with the `for` change, so this took the list route.
* [x] A `lambda` body is a sequence. Several expressions are evaluated in order
      and the last is returned, so a multi-statement function no longer needs an
      explicit `begin` - the odd one out among the body forms is closed. The
      form stores a body of two or more expressions as a synthesized
      `(begin ...)`, reusing that form's sequencing and scope rather than
      teaching `vm_call_lambda` a new shape; a single-expression body is stored
      bare and prints unchanged. The `begin` symbol is interned by name the way
      the parser interns `quote`, so it stays out of `X_LANGUAGE_SYMBOLS`.
* [x] `for` reads its binding target by shape, not a marker. `(for value list
      ...)` binds each element, `(for (key value) list ...)` binds the 0-based
      index and the element, and a bare symbol in the source position is
      unambiguous because the target is a `Symbol` or a two-`Symbol` list. The
      `In` keyword is gone, and with it the last use of the reserved `In` symbol
      left `X_LANGUAGE_SYMBOLS`.
* [x] REPL. `sparkle` with no argument reads a line, evaluates it and prints
      what it came to, until end of input. Bindings accumulate, and neither a
      runtime error nor a parse error ends the session. One line is one chunk
      of source, so several expressions on a line all run and an unbalanced
      line is an ordinary parse error - there is no multi-line continuation.
      Prompts go to stderr, so stdout carries only what the session produced
      and a piped session stays machine-readable. A Nil result prints nothing.
* [x] Diagnostics flush stdout before writing to stderr. stderr is unbuffered
      and stdout is block buffered once it is not a terminal, so a report used
      to jump ahead of the output it describes whenever either stream was
      redirected. Present in the file mode all along; invisible because the
      tester compares the two streams separately.
* [x] CI runs the contract on every push: the release build, the suite under
      ASan/UBSan with `halt_on_error` so undefined behaviour fails the job
      rather than printing past it, and a clang build so `-Werror` sees two
      compilers. `.github/workflows/ci.yml`.
* [x] Special forms are registered, not patched in. `special_forms_attach` used
      to overwrite `SPECIAL_FORMS[i].keyword` with an interned pointer, which
      made a constant table into consumed global state and meant a second VM
      with its own interner invalidated the first one's keywords.
      `vm_register_special_form` now interns into `vm->special_forms` the way
      `vm_register_builtin` registers builtins, and `vm->try_special` is gone:
      the VM holds the pairs and looks them up itself, still naming no form.
      *(verified: `SPECIAL_FORMS` is in `.rodata`; three independent
      interner/GC/VM triples registering the same table run `let`, `if`,
      `for ... In` and variadic `lambda` correctly under ASan/UBSan)*
* [x] `-Wmissing-prototypes` is on, and every function is either declared in a
      header or `static` - 77 were neither, including the ones generated inside
      the builtin macros. A layer's public surface is now enforced by the
      compiler rather than by discipline; `builtins_string.c` went from 16
      exported symbols to 0. `try_dispatch_special_form` turned out to be
      unused outside its own file and left `special_forms.h` entirely.
* [x] Language keywords left the string interner. `PREBUILTS` and
      `si->prebuilt` are gone, so `StringInterner` is a plain utility again;
      the names live as Symbol objects in `vm->symbols`, reached through
      `VM_SYM`. `parser_read_quote` interns `"quote"` directly, being under
      `core/` where `vm.h` is out of reach. `write_expr` takes the VM and
      recognises a quoted list by `StringName` identity instead of `strcmp`.
* [x] `.clang-format` names the macros it describes. The `Macros:` entries
      still said `LIST_FOREACH`/`END_LIST_FOREACH` after the rename to
      `OBJ_LIST_FOREACH`, so every `OBJ_LIST_FOREACH` body was formatted as
      unindented statements following a call.
* [x] The `rkl` fossil is gone: builtins are `spk_*` and source files are
      `.spk`, including the paths embedded in `tests/negative/*.err`.
* [x] `builtins_arithmetic_logic.c` split into `builtins_arithmetic.c`,
      `builtins_logic.c` and `builtins_control.c` - `eval`, `throw` and `apply`
      were neither arithmetic nor logic.
* [x] One name per concept: `Node` is gone in favour of `Object`, and `expr` is
      kept only where the value really is an expression. Object fields go
      through the `OBJ_*` accessors everywhere, `OBJ_AS` covering the single
      whole-union assignment in `vm_alloc`.
* [x] Prefixes match their file: everything in `vm_*.c` is `vm_*`, and the
      lexer and parser internals are `lexer_*`/`parser_*` (`lexer_read_token`,
      `parser_read_expr`, ...) instead of `lex_*`/`parse_*`.
* [x] `_Noreturn` replaces `__attribute__((noreturn))` on `vm_recover`, and
      `__attribute__((cold))` came off `vm_expect`/`vm_expect2`: they run on
      the way into every builtin, so calling them cold misinformed the
      optimizer about the hot path.
* [x] `nil?` removed as a hand-written duplicate of the generated `?NIL`.
* [x] The string buffer invariant says what actually holds: `malloc`'d, at
      least `size` bytes, never zero-sized. `cast_to_string` and `spk_input`
      hand over a `CharDA` buffer sized by capacity, so the old "at least
      `size + 1` bytes" claim in `object.h`, `gc_alloc_string_own` and
      `cast_to_string` was false whenever the size landed on the capacity.
* [x] The VM survives repeated load/run cycles, which a REPL needs: the root
      scope and its globals outlive both errors and collections.
      `vm_load_instructions` copies the caller's array instead of aliasing it -
      aliasing left `vm_mark` walking a freed buffer after an aborted run
      *(verified: heap-use-after-free under ASan with the aliasing version)*.
      `vm_alloc` initialises `instructions`, and `vm_run` clears `is_err` and
      `exception` on entry so they describe one run rather than the VM.
* [x] Object fields are reached through `OBJ_*` accessors. The bare `BOOL`,
      `STRING`, `LIST` and friends were the most collision-prone names possible
      in a public header.
* [x] Layers match their description: `sparkle_core` -> `core`, `sparkle_impl`
      -> `lang`, and `register_builtins` left `core` for `lang`, which it broke
      by taking a `VM *`. Nothing under `core/` includes `vm.h` now.
* [x] `read_file` checks `fseek`, `ftell` and `malloc` for real. The allocation
      was guarded by an `assert`, which `NDEBUG` drops in release builds.
      Usage errors go to stderr.
* [x] The `UNREACHABLE` macro and `utils.h` are gone, replaced at each site by
      `assert(false && "UNREACHABLE")`. It is a debug aid and stays one.
* [x] `Specification.md` covers the whole builtin surface, and its Evaluation, Exception and Standard Library sections are written.
* [x] `throw` raises any Symbol as an exception, including kinds the program defines.
* [x] `neg`, `abs`, `min`, `max`, `pow`, `sqrt`, `floor`, `ceil`, `round`.
* [x] `apply` - call a callable with an argument list.
* [x] `str-split`, `str-join`, `str-replace`, `str-trim`, `str-upper`, `str-lower`.
* [x] Numeric literals match the specified grammar: leading point, exponent, and the 0b/0o/0x radix forms.
* [x] `int` and `float` convert numbers and numeric strings.
* [x] `for` iterates a `List`: `(for value In list ...)` and `(for key value In list ...)`.
* [x] `input` reads a line from standard input; tests feed it through `.in` files.
* [x] `try` implements its specified form: `(try ExceptionSymbol expr1 expr2 ...)`.
* [x] Two error handler invocations behave differently: one duplicates the expression before entering the frame, the other does not.
* [x] Formatted printing function (`print` with `$N` placeholders).
* [x] `set` replaces all symbol definition simultaneously.
* [x] Multiline comments (`/* ... */`, nesting, between tokens).
* [x] `vm_build_string` must only copy the provided data.
* [x] Lambda definitions with duplicated argument names raise `VALUE_EXCEPTION`.
* [x] `'x` syntax when printing quoted values.
* [x] `str` builtin - cast any object to its printed representation.
* [x] Basic `String` manipulation.
    * [x] String escape sequences.
* [x] Proper type checking for all built-in functions.
* [x] Ref equality for compound objects.
* [x] Functions that accept `List`s MUST check for it.
* [x] `Float` literals.
* [x] Exception call when trying to evaluate an improper list.
* [x] Full parser & Lexer refactoring.
* [x] In `Readme.md`, split `Internal Design` section into `Language design` and `Interpreter design`.
* [x] `String` data type.
* [x] Flow control logical forms (`and`, `or`).
* [x] Evaluate improper lists properly :).
* [x] Rewrite special forms handling.
* [x] Factor out exceptions definition. 
* [x] Which kind of lisp is that.
* [x] Write about capitalized self-evaluating symbols.
* [x] Move error messaging to the `io.h` module.
* [x] Type checking functions. (`Integer?`, `Cons?`, `List?`...)
* [x] Type checking & exception throwing on incorrect data types in built-in functions.
* [x] Lists & Strings
    * [x] `map`, `filter`
    * [x] `length`
* [x] Invalid parser output when fed an empty file.
* [x] `'` at the end of file is handled improperly.
* [x] `&` and `|` logical functions.
* [x] `Nil` type as singleton.
* [x] `set` For variable mutation.
* [x] `while` Loops and loops in general.
* [x] `begin` Scoped lexical blocks & statement blocks.
* [x] Logical predicates (`=`, `>`, `<`, ..., `not`).
* [x] Arithmetic for `Integer` and `Float`. (`+`, `-`, `*`, `/`).
* [x] `Bool` data type & logical functions.
* [x] `Float` data type.
* [x] Write about `(x .)` & `(. x)` syntax.
* [x] Negative tests. Error message validation.
* [x] Evaluation time of tests.
* [x] Exception kinds, `throw` and `try`: raise a Symbol by name, catch several kinds per handler.
* [x] `Exception` data type carrying a value: `(throw Kind Value)` raises one pairing a kind Symbol with a value; `try` yields it, `exc-kind`/`exc-value` read it, and it prints as `Kind: Value`. A value-less exception stays a bare kind Symbol.
* [x] `while` takes a multi-expression body and evaluates it in a fresh scope per iteration, the same rule `for` has - the loops no longer disagree, and a `while` body needs no `begin`.
* [x] Introduce `StringName` data type for constant time string comparison.
* [x] Assertions on values on stack in `eval_` functions.
* [x] Exception reporting on all failed form calls (`lambda`, `if`, `let`, `quote`...)
* [x] `if` Statement with optional else branch.
* [x] Refactor `gc_mark_node` to be iterative. 
* [x] Support for quotation syntax (`'a`).
* [x] Dotted pair construction (`(a . b)`).
* [x] Arity check for functions and lambdas.
* [x] Variadic functions support using dotted-tail notation (`(lambda (x . rest) ...)`).
    * [x] Implement variadic lambda declaration
    * [x] Implement `(lambda args ...)` syntax for variadic-only lambdas.
    * [x] Implement exception calls when improper lambda definition/call is used.

