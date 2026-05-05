# Sparkle Language Specification

This document defines the semantics of Sparkle as a development & documental reference.

## Program model

- A **program** is a sequence of **s-expressions**.
- Expressions are evaluated in order, top to bottom.
- Sparkle uses strict evaluation (call-by-value): in ordinary function calls, arguments are evaluated before the call.
- Argument evaluation order is left-to-right.

## Core Types

Sparkle has the following types:

- `Nil`
- `Bool`
- `Integer`
- `String`
- `Symbol`
- `Cons`
- `Lambda`
- `Builtin`

`()` is exactly `Nil`.

### Nil

Represents the absence of a value.
**Proper list** terminator (the empty list is `Nil` itself). Defined as `Nil` or `()`.

### Bool

Represents a boolean value. `True` or `False`.
Use `(? value)` to explicitly cast any value to `Bool`.

- `Nil` → `False`
- `Integer` → `False` if `0`, otherwise `True`
- `String` → `False` if empty, otherwise `True`
- `Cons` → always `True`- `Lambda`, `Builtin` → always `True`

### Symbol

An object representing an identifier.
When evaluated, returns the value bound in the scope.
When unevaluated (e.g. via `quote`), represents itself - just a name, not a reference to anything.

```lisp
(let x 12345)

x                       ; 12345
(quote SymbolIsItself)  ; SymbolIsItself
```

### Integer

A signed integer value. `42`, `-7`, `+3`.

### String

An immutable sequence of characters. `"hello"`, `"world"`.

### Cons Cells & Lists

A **Cons Cell** is an ordered pair of two values. Its "left" and "right" slots are named `car` and `cdr` respectively.

* Use the `cons` function to build a new cell. 
* Use the `car` and `cdr` functions to retrieve values from the slots.
* Use `setcar` and `setcdr` to update the values within an existing cell.

#### Cons Cells Syntax

* `(x . y)` A cell where `car` is `x` and `cdr` is `y`.
* `(x .  )` Evaluates as `(x . Nil)`.
* `(  . x)` Evaluates as `(Nil . x)`.

```lisp
(cons 1 2)        ; a pair
(car (cons 1 2))  ; 1
(cdr (cons 1 2))  ; 2
```

#### Lists

Lists are not a separate type - they are a convention built on nested `Cons` cells.
A **proper list** is a chain of Cons cells terminated by `Nil`.

#### Lists syntax

A sequence of values enclosed in parentheses defines a proper list. In this form, the interpreter automatically terminates the chain with Nil.
* **Example**: `(a b c d e)` defines a Nil-terminated list.

```lisp
; Lists are nested cons cells terminated by Nil
(cons 1 (cons 2 (cons 3 Nil)))  ; (1 2 3)

(car '(1 2 3 4 5))              ; 1
(cdr '(1 2 3 4 5))              ; (2 3 4 5)
```

An **improper list** is a **Cons** chain where the last `cdr` is not `Nil`.

A sequence of values enclosed in parentheses where the finl element is preceded by a dot defines an improper list.
In this form, the list is terminated by the object following the dot.
Example: `(1 2 3 4 . 5)` defines an improper list with `5` as its terminal value.

```lisp
(cons 1 2)           ; (1 . 2)   - improper
(cons 1 (cons 2 3))  ; (1 2 . 3) - improper
(1 2 3 . 4)          ; (1 2 3 . 4) - improper
```

### Built-in functions

Functions provided by the language itself, implemented in C.
Behave identically to user-defined functions - first-class objects, can be passed around and stored.

### Lambda functions

An anonymous function. A callable object that can be stored and passed around.

```lisp
(let add (lambda (x y) (+ x y)))
(add 1 2)  ; 3

; Closures
(let make-adder (lambda (x) (lambda (y) (+ x y))))
(let add5 (make-adder 5))
(add5 3)  ; 8
```

### Truthness and Bool Conversion

Any value can be converted to `Bool` via `(? x)`.

- `Nil` -> `False`
- `Integer` -> `False` iff value is `0`
- `String` -> `False` iff empty string `""`
- `Cons`/`Lambda`/`Builtin`/`Symbol` -> Always `True`

```lisp
(? 0)    ; False
(? 1)    ; True
(? Nil)  ; False
(? "")   ; False
(< 2 3)  ; True
(= 1 2)  ; False
```

## Special Forms & Flow Control

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.
Special forms are usually used for flow control or state mutation.

### let

`let` introduces a lexical bining in the current scope.
Bining an already binded symbol is a runtime error.

```lisp
(let x 42)
(let add (lambda (x y) (+ x y)))
```

### set

`set` updates an existing binding.
Using set on an unbound name is a runtime error.

```lisp
(let x 1)
(set x 2)
x ; 2
```

### if

Conditional expression. Evaluates the condition, then either the true or false branch.
Automatically casts the condition to `Bool`.

```lisp
(if (= x 0)
    "zero"
    "non-zero")
```

### lambda

Creates a Lambda object.

```lisp
(lambda (x y) (+ x y))
(lambda (x) (lambda (y) (+ x y)))  ; curried

(let square (lambda (x) (* x x)))
(square 5)  ; 25
```

### quote

Returns its argument unevaluated.

```lisp
(quote x)        ; x       - a symbol, not its value
(quote (1 2 3))  ; (1 2 3) - a list, not a call
```

## Function definition and calls

Sparkle supports first-class anonymous functions defined with the `lambda` keyword.
It is possible to define variadic lambda functions.
Combined with `let` form, can define named functions.

```lisp
(let square (lambda (x) (* x x)))
(print (square 5))             ; 25
((lambda (x y) (+ x y)) 11 12) ; 23
```

### Closures

Lambda functions capture their defining scope, allowing them to reference variables from the enclosing environment.

```lisp
(let adder_factory (lambda (x) (lambda (y) (+ x y))))

(let add3 (adder_factory 3))
(let add5 (adder_factory 5))

(print (add3 (add5 7)))  ; 15
```

### Higher-order Functions

Functions can be passed as arguments and returned as values.

```lisp
(let map (lambda (f l)
    (if l
        (cons (f (car l)) (map f (cdr l)))))
        NIL)

(let double (lambda (x) (+ x x)))
(print (map double (range 1 5)))  ; (2 4 6 8 10)
```

## Error handling

This part of the language is currently being developed and is not yet fully specified.

