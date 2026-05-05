# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] Multiline comments.
* [ ] String escaping sequences.
* [ ] More descriptive parsing errors (on expected tokens).

## Special Forms & Control Flow 

* [ ] `set` For variable mutation.
* [ ] `if` Statement with optional else branch.
* [ ] `while` Loops and loops in general.
* [ ] `begin` Scoped lexical blocks & statement blocks.
* [ ] Flow controling logical forms (`and`, `or`).

## Optimizations

* [ ] Refactor `gc_mark_node` to be iterative. 
* [ ] Introduce `StringName` data type for constant time string comparison.
    * [ ] Hash tables for fast search in scopes.
* [ ] `Nil` type as singleton.
* [ ] Arena allocations instead of an linked list in GC.
* [ ] An attempt/research on TCO.

## Language Semanatics

* [ ] `Bool` data type & logical functions.
* [ ] `Float` data type.
* [ ] `String` data type & basic string manipulation.
* [ ] `Exception` data type for expressive error reporting & powerful catch mechanism.
* [-] Variadic functions support using dotted-tail notation (`(lambda (x . rest) ...)`).
    * [x] Implement variadic lambda declaration
    * [ ] Implement expetion calls when improper lambda definition/call is used.
* [ ] Exception reporting on all failed form calls (`lambda`, `if`, `let`, `quote`...)
* [ ] Type checking & exception throwing on incorrect data types in built-in functions.
* [ ] Macro support

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

## Documentation & Presentation

* [ ] Which kind of lisp is that.
* [ ] Write about `(x .)` & `(. x)` syntax.

## Code base, bugs, & fixes

* [ ] Assertions on values on stack in `eval_` functions.
* [ ] `'x` syntax when printing quoted values.
* [ ] Two error handler invocations behave differently: one duplicates the expression before entering the frame, the other does not.

## Tooling

* [ ] Negative tests. Error message validation.
* [ ] Evaluation time of tests.

## UX

* [ ] Proper interpreter interaction.
* [ ] REPL.

## Done

* [x] Support for quotation syntax (`'a`).
* [x] Dotted pair construction (`(a . b)`).
* [x] Arity check for functions and lambdas.

