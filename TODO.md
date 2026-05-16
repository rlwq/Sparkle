# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] Multiline comments.
* [ ] String escaping sequences.
* [ ] More descriptive parsing errors (on expected tokens).
* [ ] `Float` literals.

## Special Forms & Control Flow 

* [ ] Flow control logical forms (`and`, `or`).

## Optimizations
 
* [ ] Integer Interning.
* [ ] Hash tables for fast search in scopes.
* [ ] Arena allocations instead of an linked list in GC.
* [ ] An attempt/research on Tail Call Optimization.

## Language Semantics

* [ ] `String` data type & basic string manipulation.
* [ ] Type checking & exception throwing on incorrect data types in built-in functions.
* [ ] Macro support.
* [ ] Lambda definitions with duplicated argument names.
* [ ] Functions that accept `List`s MUST check for it.

## Base Library

* [ ] Math library.
* [ ] Some kind of structure equality and ref equality for compound objects.
* [ ] I/O
    * [ ] Formatted printing function 
    * [ ] Raw printing function for any objects
    * [ ] Input
    * [ ] File reading/writing
* [ ] Lists & Strings
    * [x] `map`, `filter`
    * [ ] `fold`
    * [ ] `length`, `drop`, `take`
    * [ ] `append`, `push`, `reverse`
    * [ ] `find`, `pop`, `replace`
* [ ] Type checking functions. (`Integer?`, `Cons?`, `List?`...)
* [ ] System & OS
    * [ ] `exit`, `argc/argv`...
* [ ] Proper type checking for all built-in functions.

## Documentation & Presentation

* [ ] Which kind of lisp is that.
* [ ] Write about capitalized self-evaluating symbols.

## Code base, bugs, & fixes

* [ ] Move error messaging to the `io.h` module.
* [ ] `'x` syntax when printing quoted values.
* [ ] Two error handler invocations behave differently: one duplicates the expression before entering the frame, the other does not.
* [ ] Rewrite special forms handling.
* [ ] Exception call when trying to evaluate an improper list.
* [ ] Full parser & Lexer refactoring.

## Tooling


## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.

## Done

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

