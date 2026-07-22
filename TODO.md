# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] More descriptive parsing errors (on expected tokens).
* [ ] Escape syntax for symbols that contain delimiters.

## Special Forms & Control Flow 

* [ ] Early exit: no `break`, `continue` or `return`, so a loop cannot stop
      before its condition flips and a function cannot return from a branch.
* [ ] `cond`-style dispatch on a value rather than on a chain of predicates.
* [ ] No unwind protection: an exception unwinds past whatever cleanup the
      program wanted to run. Once file handles exist this stops being academic.

## Error Reporting

**Next task.** Not one job but four independent ones, listed in the order they
are worth doing: each is cheaper than the one after it, and each leaves the next
somewhere to plug into. Two numbers worth having in hand before starting.

`sizeof(Object)` is 64 bytes today, laid out `kind` at 0, `marked` at 4, three
bytes of padding, `heap_next` at 8, the union at 16. Adding a position costs:
`uint16` byte offset **0 bytes** (it fits the padding hole, but caps a source
file at 64K), `uint32` byte offset **+8**, a `TextPos` of two `size_t`
**+16**. That is per object, every Integer included.

The sixty places that raise an exception are 33 `VALUE_EXCEPTION`, 17
`TYPE_EXCEPTION`, 4 `ARITY`, 3 `UNDEFINED`, 2 `UNCALLABLE`, 1 `REBINDING` - so
fifty of sixty raise one of the two messages that say nothing whatsoever. Which
is why piece 2 buys more than piece 4 for a fraction of the work.

* [ ] **1. Parser errors carry no position, though the position is already in
      hand.** `diag_parser` opens with `(void)parser;` and prints a constant
      string, while `parser->cursor` points at the token that failed and every
      token carries `pos`. The cheapest item in this file: stop discarding what
      is already there. Subsumes "More descriptive parsing errors" above.

* [ ] **2. Messages do not say what went wrong, though the answer is in scope
      where the exception is raised.** `vm_scope_get` holds the name it failed
      to find and reports "Symbol has no definition". `vm_expect` holds both the
      expected type mask and the offending object and reports "Function expected
      some other object type". `vm_call_lambda` holds both arities. Nothing new
      has to be collected - the information exists and is thrown away.
      Wants a detail channel on the VM beside `vm->exception`: a fixed buffer
      rather than an allocation, since allocating while unwinding is asking for
      trouble, and a `vm_recover_fmt` beside `vm_recover` so the sixty existing
      sites keep compiling and improve one at a time. Kind names generate from
      `X_KINDS`, the way `token_kind_names` already does for tokens.
      Detail for the reporter only. A payload the program can catch and inspect
      is a language change, tracked separately under Language Semantics.

* [ ] **3. No call trace.** "Failed in `foo`" without "called from `bar`, called
      from line 12" is half an answer. Wants the explicit control stack that the
      deep-recursion item under Code base also wants - one structure, both
      problems: a depth limit that raises instead of a segfault, and a trace to
      print. Do them in one go.

* [ ] **4. Runtime errors carry no position, and objects have nowhere to put
      one.** The expensive, architectural piece; left last because by then the
      trace and the detail channel already exist, so there is somewhere to put
      a position rather than a format to invent alongside it.
      Store a byte offset and derive `line:column` when reporting, by counting
      newlines: it happens once, at failure, where speed is irrelevant, and it
      halves the cost per object. Take the `uint32`; the free `uint16` buys a
      silent 64K ceiling, which is worse than eight bytes.
      Only the parser sets it. Objects built at runtime have no position and
      should not pretend to, and the scheme survives singletons because the
      parser allocates a fresh object per token and shares nothing.
      New requirement to write down where it is enforced: the source text has
      to be alive when the report is made. It is today, since `interp_eval`
      reports before returning while `src` is still the caller's, but that is
      currently a coincidence rather than a stated contract.

## Optimizations
 
* [ ] Integer Interning.
* [ ] Arena allocations instead of a linked list in GC.
* [ ] Scope lookup is a linear scan: `scope_get` walks every binding of every
      enclosing scope on each symbol evaluation. Index it.
* [ ] String interning is a linear scan over every string interned so far
      (`si_getn`), so interning is O(n) in the program's symbol count. Hash it.
* [ ] Special-form dispatch scans the whole table on every list evaluation
      (`vm_try_special_form`). The scan is now over `vm->special_forms`, so it is
      per-VM data rather than a global table.

## Language Semantics

* [ ] Builtins may be shadowed, like Python: a later binding of a builtin's name
      takes precedence in its scope. Decided; what remains is saying so in
      `Specification.md` and confirming the interpreter lets a builtin's name be
      rebound rather than raising `REBINDING_EXCEPTION`.
* [ ] Exceptions carry nothing. `throw` raises a bare Symbol, so a failure
      cannot report which value or index caused it - the kind is the whole
      message. Planned shape, deferred: a new `Exception` type pairing a `Symbol`
      kind with a message value, raised as `(throw Kind Value)` with the value
      optional and reported as `Kind: Value`; `try` still matches on the kind and
      gains a way to bind the value. Re-raise is deliberately left out - with no
      polymorphic exception objects, discriminating by kind (now that `try`
      accepts several) is enough. Deliberately not part of Error Reporting piece
      2, which only routes detail to the reporter and leaves `try` alone.
* [ ] Integer overflow is unspecified. `Integer` is a C `long long` and signed
      overflow is undefined behaviour, so `(* 99999999999 99999999999)` is
      whatever the optimizer decides. Pick wrapping, saturation, promotion or
      an exception, then say so in `Specification.md`.
* [ ] Float printing does not round-trip. The form is now specified - fixed
      point, six decimals, never an exponent - so the loss is at least written
      down rather than merely suffered, but a magnitude below `0.0000005` still
      prints as `0.000000` and `inf` and `nan` do not read back at all. Wants a
      shortest-round-trip form, and literals the reader accepts for the
      non-finite values.

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

## Documentation & Presentation

* [ ] `Readme.md` does not open with what the language looks like. A reader
      deciding whether to keep scrolling wants a program in the first screen.
* [ ] A tutorial. `Specification.md` defines the language, which is not the
      same as teaching it: there is no path from zero to a working program
      short of reading the whole thing. This is what `README.md` should carry,
      now that it is the only document written for a reader rather than for an
      implementer.
* [ ] `examples/` is thin. Each one should be a program worth reading, not a
      feature demo.
* [ ] Nothing keeps `Specification.md` honest as the language moves. A
      checklist in the test suite, or a test that reads the document. The
      merge that removed `Sparkle.md` found the gap the hard way: it was
      missing all thirteen string functions, claimed `(+)` with no arguments
      returns `0` when it raises `ARITY_EXCEPTION`, and carried a worked
      example of a multi-expression `lambda` body that did not parse until the
      body became a sequence.
* [ ] No version number and no changelog, so there is nothing to point at when
      something changes under a user.

## Code base, bugs, & fixes

* [ ] Deep recursion overflows the C stack and kills the process instead of
      raising something catchable. Each Sparkle-level call costs several C
      frames (`vm_eval_object` -> `vm_eval_list` -> `vm_call` -> `vm_call_lambda`
      -> `vm_eval_object`). Wants tail-call elimination, or a depth limit that
      raises, or an explicit control stack. The control stack is the same
      structure error reporting wants for a call trace - see Error Reporting,
      piece 3, and do them together.
      *(verified: depth 10000 fine, 50000 overflows the stack under `make debug`)*
* [ ] Allocation failure is unchecked in release. `da_init` and `da_push` guard
      `malloc` and `realloc` with `assert`, which `NDEBUG` compiles out, so the
      release build writes through a null pointer under memory pressure rather
      than failing. The GC constructors inherit this. *(verified:
      `dynamic_array.h:22` and `:31` are asserts; release defines `NDEBUG`)*
* [ ] Nothing bounds interpreter memory. A runaway program takes the machine
      down with it instead of raising.
* [ ] `X_LANGUAGE_SYMBOLS` still lives in `vm.h`, so the VM core spells out
      words only the language gives meaning to (`Nil`, `True`, `False`,
      `quote`, `Var`, `In`). It belongs in `lang/`, registered into the VM the
      way special forms now are - `vm_register_special_form` already shows the
      shape. The self-evaluating-symbol rule in `vm_eval_symbol` (capitalized
      symbols evaluate to themselves) has to move with it, since that is the
      only thing reading `Nil`/`True`/`False`. Moving it out of the interner
      was the first half of this.

## Tooling

* [ ] No module system: a program is a single file, with no `import` or `load`,
      so nothing can be split up or reused across programs.
* [ ] Benchmark suite - the test runner times each case, but nothing tracks
      whether the interpreter is getting faster or slower.
* [ ] Editor support: syntax highlighting, at least a `.spk` grammar.
* [ ] No install target - the binary only exists at `./build/sparkle`.
* [ ] A trace mode that prints evaluation steps. The interpreter is a teaching
      artifact as much as a tool, and it cannot show its own work.

## UX

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

## Done

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
* [x] `Exception` data type for expressive error reporting & powerful catch mechanism.
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

