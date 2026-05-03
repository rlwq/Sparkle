# TODO

This document outlines the features and fixes planned for Sparkle.

## Syntax & Parser

* [ ] Support for quotation syntax (`'a`).
* [ ] Dotted pair construction (`(a . b)`).

## Special Forms & Control Flow 

* [ ] `set` For variable mutation.
* [ ] `if` Statement with optional else branch.
* [ ] `while` Loops and loops in general.
* [ ] `begin` Scoped lexical blocks & statement blocks.

## Optimizations

* [ ] Refactor `gc_mark_node` to be iterative. 
* [ ] Introduce `StringName` data type for constant time string comparison.

## Language Semanatics

* [ ] `Bool` data type & logical functions.
* [ ] `String` data type & basic string manipulation.
* [ ] `Exception` data type for expressive error reporting & powerful catch mechanism.
* [ ] Variadic functions support using dotted-tail notation (`(lambda (x . rest) ...)`).
* [ ] Distinct error types and descriptive messages.

