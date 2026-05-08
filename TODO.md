# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] Multiline comments.
* [ ] String escaping sequences.
* [ ] More descriptive parsing errors (on expected tokens).
* [ ] `'` at the end of file is handled inproperly.

## Special Forms & Control Flow 

* [ ] `set` For variable mutation.
* [ ] `while` Loops and loops in general.
* [ ] `begin` Scoped lexical blocks & statement blocks.
* [ ] Flow controling logical forms (`and`, `or`).

## Optimizations

* [ ] Hash tables for fast search in scopes.
* [ ] `Nil` type as singleton.
* [ ] Arena allocations instead of an linked list in GC.
* [ ] An attempt/research on TCO.

## Language Semanatics

* [ ] `Bool` data type & logical functions.
* [ ] `Float` data type.
* [ ] `String` data type & basic string manipulation.
* [ ] Type checking & exception throwing on incorrect data types in built-in functions.
* [ ] Macro support.
* [ ] Lambda definitions with duplicated argument names.

## Base Library

* [ ] Arithmetic for `Integer` and `Floor`. (`+`, `-`, `*`, `/`).
* [ ] Maths library.
* [ ] Logical predicates (`=`, `>`, `<`, ..., `not`).
    * [ ] Some kind of structure equality and ref equality for compound objects.
* [ ] I/O
    * [ ] Formatted printing function 
    * [ ] Raw printing function for any objects
    * [ ] Input
    * [ ] File reading/writing
* [ ] Lists & Strings
    * [ ] `map`, `filter`, `fold`
    * [ ] `length`, `drop`, `take`
    * [ ] `append`, `push`, `reverse`
    * [ ] `find`, `pop`, `replace`
* [ ] Type checking functions. (`Integer?`, `Cons?`, `List?`...)
* [ ] System & OS
    * [ ] `exit`, `argc/argv`...
* [ ] Proper type checking for all built-in functions.

## Documentation & Presentation

* [ ] Which kind of lisp is that.
* [ ] Write about `(x .)` & `(. x)` syntax.

## Code base, bugs, & fixes

* [ ] `'x` syntax when printing quoted values.
* [ ] Two error handler invocations behave differently: one duplicates the expression before entering the frame, the other does not.
* [ ] Exception call when trying to evaluate an improper list.
* [ ] Invalid parser output when feeded an empty file.

## Tooling


## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.

## Done

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

