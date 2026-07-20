# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] More descriptive parsing errors (on expected tokens).

## Special Forms & Control Flow 


## Optimizations
 
* [ ] Integer Interning.
* [ ] Arena allocations instead of a linked list in GC.

## Language Semantics

## Base Library

* [ ] Structure equality. 
* [ ] I/O
    * [ ] Input

## Documentation & Presentation


## Code base, bugs, & fixes

## Tooling


## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.

## Done

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

