# Sparkle programming language

## Types

### Nil

Represents the absence of a value.
**Proper list** terminator (the empty list is `Nil` itself). Defined as `Nil` or `()`.

### Bool

Represents a boolean value. `True` or `False`.
Use `(? value)` to explicitly cast any value to `Bool`.

- `Nil` → `False`
- `Integer` → `False` if `0`, otherwise `True`
- `String` → `False` if empty, otherwise `True`
- `Cons` → always `True`
- `Lambda`, `Builtin` → always `True`

```lisp
(? 0)    ; False
(? 1)    ; True
(? Nil)  ; False
(? "")   ; False
(< 2 3)  ; True
(= 1 2)  ; False
```

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

### Cons

An ordered pair of two values.

```lisp
(cons 1 2)        ; a pair
(car (cons 1 2))  ; 1
(cdr (cons 1 2))  ; 2

; Lists are nested cons cells terminated by Nil
(cons 1 (cons 2 (cons 3 Nil)))  ; (1 2 3)
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

### Lists

Lists are not a separate type - they are a convention built on `Cons` cells.
A **proper list** is a chain of Cons cells terminated by `Nil`.

```lisp
(cons 1 (cons 2 (cons 3 Nil)))  ; (1 2 3)
(car '(1 2 3 4 5))              ; 1
(cdr '(1 2 3 4 5))              ; (2 3 4 5)
```

An **improper list** is a Cons chain where the last `cdr` is not `Nil`.

```lisp
(cons 1 2)           ; (1 . 2)   - improper
(cons 1 (cons 2 3))  ; (1 2 . 3) - improper
```

## Special Forms

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.

### let

Binds a value to a name in the current scope.

```lisp
(let x 42)
(let add (lambda (x y) (+ x y)))
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

## Lambda Functions

Sparkle supports first-class anonymous functions defined with the `lambda` keyword.
Combined with `let` form, can define named functions.

```lisp
(let square (lambda (x) (* x x)))
(print (square 5))             ; 25
((lambda (x y) (x + y)) 11 12) ; 23
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
        NIL
        (cons (f (car l)) (map f (cdr l))))))

(let double (lambda (x) (+ x x)))
(print (map double (range 1 5)))  ; (2 4 6 8 10)
```

### Macros

This part of the language is currently being developed and is not yet fully specified.

### Error handling

This part of the language is currently being developed and is not yet fully specified.

### Module system

This part of the language is currently being developed and is not yet fully specified.
