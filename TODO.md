# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] Multiline comments.
* [ ] More descriptive parsing errors (on expected tokens).

## Special Forms & Control Flow 


## Optimizations
 
* [ ] Integer Interning.
* [ ] Arena allocations instead of a linked list in GC.

## Language Semantics

* [ ] Basic `String` manipulation.
    * [ ] String escaping sequences.
* [ ] Lambda definitions with duplicated argument names.
* [ ] `set` replaces all symbol definition simultaneously.

## Base Library

* [ ] Structure equality. 
* [ ] I/O
    * [ ] Formatted printing function 
    * [ ] Raw printing function for any objects
    * [ ] Input

## Documentation & Presentation


## Code base, bugs, & fixes

* [ ] Two error handler invocations behave differently: one duplicates the expression before entering the frame, the other does not.
* [ ] `'x` syntax when printing quoted values.

## Tooling


## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.

## Done

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
* [x] Arithmetic for `Integer` and `Floor`. (`+`, `-`, `*`, `/`).
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

