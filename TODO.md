# TODO

This document outlines the features and fixes planned for Sparkle.

Items marked *(verified)* were reproduced against the current build rather than
assumed - the reproduction is written next to them.

## Syntax & Parser

* [ ] More descriptive parsing errors (on expected tokens).
* [ ] Quasiquote, unquote and splicing (`` ` ``, `,`, `,@`). Prerequisite for
      macros: without it a macro body has to build lists by hand.
* [ ] Escape syntax for symbols that contain delimiters.

## Special Forms & Control Flow 

* [ ] Macros. The one Lisp feature the language is missing outright; needs
      quasiquote first.
* [ ] Early exit: no `break`, `continue` or `return`, so a loop cannot stop
      before its condition flips and a function cannot return from a branch.
* [ ] `cond`-style dispatch on a value rather than on a chain of predicates.

## Optimizations
 
* [ ] Integer Interning.
* [ ] Arena allocations instead of a linked list in GC.
* [ ] Scope lookup is a linear scan: `scope_get` walks every binding of every
      enclosing scope on each symbol evaluation. Index it.
* [ ] String interning is a linear scan over every string interned so far
      (`si_getn`), so interning is O(n) in the program's symbol count. Hash it.
* [ ] Special-form dispatch scans the whole table on every list evaluation
      (`try_dispatch_special_form`).

## Language Semantics

* [ ] No way to raise an exception from user code. `try` can catch, but nothing
      can throw: a program cannot signal its own errors, only the six built-in
      kinds ever occur. *(verified: no `throw`/`raise`/`error` is registered)*
* [ ] User-defined exception kinds, once throwing exists.
* [ ] Decide whether shadowing a builtin is allowed, and say so.

## Base Library

* [ ] Structure equality. 
* [ ] `neg` is documented in `Sparkle.md` but not implemented.
      *(verified: `(neg 5)` raises UNDEFINED_EXCEPTION)*
* [ ] `apply` - call a callable with an argument list. `vm_call` already exists
      as the primitive and `vm.h` names this as its future use.
* [ ] `reduce` / `fold`.
* [ ] `sort` with a comparator - `tests/positive` hand-rolls merge and
      insertion sort because there is none.
* [ ] A keyed collection: there is no map, dictionary or set, so an association
      list walked by hand is the only lookup structure.
* [ ] Math: `abs`, `min`, `max`, `pow`, `sqrt`, `floor`, `ceil`, `round`.
* [ ] More string operations: `str-split`, `str-join`, `str-replace`, trimming,
      case conversion.
* [ ] File I/O. Standard input and output are the only streams.
* [ ] Access to command-line arguments and environment variables.
* [ ] Exit with a chosen status code.
* [ ] Clock / time.
* [ ] Random numbers.

## Documentation & Presentation

* [ ] `Evaluation Model`, `Standard Library` and `Exception Model` are empty
      headings in `Specification.md`. *(verified)*
* [ ] The `?KIND` type predicates (`?INTEGER`, `?STRING`, `?LIST`, `?LAMBDA`,
      ...) are documented nowhere. *(verified: zero mentions in either document,
      though all nine exist and work)*
* [ ] Audit the rest of the builtin surface against `Specification.md` the same
      way - `neg` was found by diffing documented names against the runtime, so
      the reverse direction is worth a pass too.

## Code base, bugs, & fixes

* [ ] Deep recursion overflows the C stack and kills the process instead of
      raising something catchable. Each Sparkle-level call costs several C
      frames (`vm_eval_node` -> `vm_eval_list` -> `vm_call` -> `call_lambda` ->
      `vm_eval_node`). Wants tail-call elimination, or a depth limit that
      raises, or an explicit control stack.
      *(verified: depth 10000 fine, 50000 overflows the stack under `make debug`)*
* [ ] Runtime errors report the file and a generic message - no line, no column,
      no call stack, no offending expression. Lexer errors already carry
      `line:column`, so the machinery exists. *(verified: `diag_vm` prints only
      `path: [RUNTIME ERROR] message`)*
* [ ] `svtod` scans for a `.` with no bound on the view, so a view without one
      reads past its end. Only the lexer calls it today, and only for a token
      that already contains a point, but nothing in its signature says so.
* [ ] `cast_to_string` says the DA buffer is always at least `size + 1` bytes;
      when the size lands exactly on the capacity there is no spare byte.
      Harmless today, since strings are a data/size pair and are never treated
      as NUL-terminated, but the comment claims an invariant that does not hold.

## Tooling

* [ ] No module system: a program is a single file, with no `import` or `load`,
      so nothing can be split up or reused across programs.
* [ ] Benchmark suite - the test runner times each case, but nothing tracks
      whether the interpreter is getting faster or slower.
* [ ] Editor support: syntax highlighting, at least a `.rkl` grammar.

## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.
* [ ] Run a program from standard input, and evaluate an expression given on the
      command line (`-e`).

## Done

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

