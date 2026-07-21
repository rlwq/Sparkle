# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] More descriptive parsing errors (on expected tokens).
* [ ] Escape syntax for symbols that contain delimiters.
* [ ] Parsed nodes carry no source position, so nothing downstream can name a
      line. Tokens have `line:column` and lose it at `parse_expr`. Prerequisite
      for runtime errors that point somewhere.

## Special Forms & Control Flow 

* [ ] `try` should catch several kinds at once:
      `(try kind1 kind2 ... In expr1 expr2 ...)`. It catches exactly one today,
      so guarding against more means nesting one `try` per kind - four deep in
      `examples/rpn.spk`, which is the first thing real code runs into.
      The `In` marker earns its place the same way it does in `for`: kinds are
      evaluated expressions, so a bare symbol is legal in both positions and
      `(try A B expr)` is otherwise indistinguishable from `(try A expr1 expr2)`.
      Note this changes existing syntax - every current `(try KIND expr)` needs
      `In` inserted, tests and both documents included.
* [ ] Early exit: no `break`, `continue` or `return`, so a loop cannot stop
      before its condition flips and a function cannot return from a branch.
* [ ] `cond`-style dispatch on a value rather than on a chain of predicates.
* [ ] `let` binds one symbol per form, so a block opens with a column of
      `let`s. Wants several bindings at once, and a decision on whether later
      ones see earlier ones.
* [ ] No unwind protection: an exception unwinds past whatever cleanup the
      program wanted to run. Once file handles exist this stops being academic.
* [ ] `try` cannot re-raise. A handler that inspects the kind and decides it is
      not the right one has no way to pass it outward.
* [ ] Mutual recursion between two lambdas is unspecified - whether the first
      body sees the second symbol depends on when the scope entry appears.
      Decide it and write it down.

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

* [ ] Decide whether shadowing a builtin is allowed, and say so.
* [ ] Exceptions carry nothing. `throw` raises a bare Symbol, so a failure
      cannot report which value or index caused it - the kind is the whole
      message. Wants a payload, and `try` wants a way to bind it.
* [ ] Integer overflow is unspecified. `Integer` is a C `long long` and signed
      overflow is undefined behaviour, so `(* 99999999999 99999999999)` is
      whatever the optimizer decides. Pick wrapping, saturation, promotion or
      an exception, then say so in `Specification.md`.
* [ ] Closure capture is unwritten: a lambda holds a `Scope *`, so it captures
      by reference and a later `set` is visible inside. That is a real choice
      and it is documented nowhere.
* [ ] Float printing does not round-trip - the printed form is not guaranteed
      to read back as the same double.

## Base Library

* [ ] Structure equality. 
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
* [ ] A tutorial: `Sparkle.md` is a reference, so there is no path from zero to
      a working program that is not reading the whole thing.
* [ ] `examples/` is thin. Each one should be a program worth reading, not a
      feature demo.
* [ ] Nothing keeps `Sparkle.md` and `Specification.md` honest as the language
      moves. A checklist in the test suite, or a test that reads the documents.
* [ ] No version number and no changelog, so there is nothing to point at when
      something changes under a user.

## Code base, bugs, & fixes

* [ ] Deep recursion overflows the C stack and kills the process instead of
      raising something catchable. Each Sparkle-level call costs several C
      frames (`vm_eval_object` -> `vm_eval_list` -> `vm_call` -> `vm_call_lambda`
      -> `vm_eval_object`). Wants tail-call elimination, or a depth limit that
      raises, or an explicit control stack.
      *(verified: depth 10000 fine, 50000 overflows the stack under `make debug`)*
* [ ] Runtime errors report the file and a generic message - no line, no column,
      no call stack, no offending expression. Lexer errors already carry
      `line:column`, so the machinery exists. *(verified: `diag_vm` prints only
      `path: [RUNTIME ERROR] message`)*
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
* [ ] REPL. **Next task.** One line per iteration, no multi-line continuation:
      an unbalanced line is an ordinary parse error, so `core/` needs no change
      and `diag_parser` works as is. A line holding several expressions already
      works - the parser yields several nodes and `vm_run` runs them all.
      The VM side is ready: load/run repeats on one VM, and the root scope
      survives both errors and collections.
      Two things still open.
      Echo: `vm_run` pops each result, so `(+ 1 2)` would print nothing.
      `write_expr` (`io.h`) can print it and now takes the `VM *` anyway, which
      suits the flag below; the question is who calls it. Leaning
      towards a `vm->echo_results` flag over wrapping input in `(print ...)`
      textually, which breaks on `(let x 1)` and on multi-expression lines, and
      over a second run loop, which would duplicate the `setjmp` recovery.
      Undecided whether `(print "hi")` should then also echo its `Nil`.
      Input: `spk_input` reads the same `stdin` the REPL reads commands from,
      so `(input)` will eat the next command.
      *(verified: `spk_input` calls `fgetc(stdin)`, `builtins_io.c:130`)*
* [ ] Run a program from standard input, and evaluate an expression given on the
      command line (`-e`).
* [ ] The CLI takes one argument and nothing else: no `--help`, no `--version`,
      and the usage line is the only thing resembling documentation.
* [ ] Exit statuses are undocumented. Today everything that fails exits 1, so a
      script cannot tell a syntax error from a runtime one.

## Done

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

