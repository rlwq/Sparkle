# Sparkle Language Specification

This document defines the semantics of Sparkle as a development & documental reference.

## Program model

* A **program** is a sequence of **s-expressions** evaluated top to bottom. Each expression is evaluated before the next begins.
* Sparkle uses strict evaluation (call-by-value): in a function call, all arguments are
evaluated left-to-right before the function is invoked.
* **Special forms** differ from function calls: they receive their arguments unevaluated and control evaluation themselves. Each special form documents which arguments it evaluates and when.

## Core Types

Sparkle has the following types: `Nil`, `Bool`, `Integer`, `Float`, `String`, `Symbol`, `Cons`, `Lambda`, `Builtin` and `Exception`.

### Nil

Represents the absence of a value.

Literal syntax: `()`.
A symbol with value `Nil` evaluates to the `Nil` object.

A **Proper list** terminator.
Represents an empty **proper list**.

```lisp 
(eval 'Nil)  ; Nil (object of type Nil)
(print ())   ; Nil
```

### Bool

Represents a boolean value. `True` or `False`.

A symbol with value `True` evaluates to a `Bool` object with value `True`.
A symbol with value `False` evaluates to a `Bool` object with value `False`.

Use `(? value)` function to explicitly cast any value to `Bool`.

Truthiness rules for casting:
* `Nil` → always `False`
* `Bool` → itself
* `Integer`, `Float` → `False` if `0`, otherwise `True`
* `String` → `False` if empty, otherwise `True`
* `Cons`, `Lambda`, `Builtin`, `Symbol`, `Exception` → always `True`

```lisp
(? 0)    ; False
(? 1)    ; True
(? Nil)  ; False
(? "")   ; False
```

### Symbol

An object representing an identifier.

Literal syntax: any sequence of non-whitespace, non-parenthesis, non-quote characters that is not a valid `Integer` or `Float` literal. Examples: `x`, `foo`, `nil?`, `+`.

When evaluated, a symbol resolves to the value bound to the name in the current scope.
`Nil`, `True`, and `False` are self-evaluating: they always produce their respective values.

When unevaluated (e.g. via `quote`), represents itself - a name, not a reference.
Symbols whose name begins with `:` evaluate to themselves: `(eval ':foo)` results in symbol `:foo`.

```lisp
(let x 42)

x                         ; 42
(quote symbol-is-itself)  ; symbol-is-itself
(= :foo :bar)             ; False
```

### Integer

A signed integer value.
Ex.: `42`, `-7`, `+3`.

### Float

A floating-point number.
Ex.: `3.14`, `-0.5`.

### String

An immutable sequence of characters.
Ex.: `"hello"`, `"world"`.

### Cons 

#### Cons pairs

A **Cons Cell** is an ordered pair of two values.
Its "left" and "right" slots are named `car` and `cdr` respectively.

Literal syntax:
* `(x . y)` - a cell where `car` is `x` and `cdr` is `y`. Shorthand for `(cons x y)`.
* `(x .  )` is parsed as `(x . Nil)`.
* `(  . x)` is parsed as `(Nil . x)`.

* Use the `cons` function to build a new cell. 
* Use the `car` and `cdr` functions to retrieve values from the slots.
* Use `setcar` and `setcdr` to update the values within an existing cell.

```lisp
(cons 1 2)        ; a pair
(car (cons 1 2))  ; 1
(cdr (cons 1 2))  ; 2
```

#### Lists

##### Proper lists

Lists are not a separate type - they are a convention built on nested `Cons` cells.
A **proper list** is a chain of Cons cells terminated by `Nil`.

Literal syntax: `(a b c d)` - elements of a list enclosed in parentheses. The `Nil` terminator is added implicitly.

```lisp
; Lists are nested cons cells terminated by Nil
(cons 1 (cons 2 (cons 3 Nil)))  ; (1 2 3)

(car '(1 2 3 4 5))              ; 1
(cdr '(1 2 3 4 5))              ; (2 3 4 5)
```

##### Improper lists

An **improper list** is a **Cons** chain where the last `cdr` is not `Nil`.

A sequence of values enclosed in parentheses where the final element is preceded by a dot defines an **improper list**.

Literal syntax: `(1 2 3 4 . 5)` defines an improper list with `5` as its terminal value.

In this form, the list is terminated by the object following the dot.

```lisp
(cons 1 2)           ; (1 . 2)      - improper
(cons 1 (cons 2 3))  ; (1 2 . 3)    - improper
(1 2 3 . 4)          ; (1 2 3 . 4)  - improper
(:a :b :c . Nil)     ; (:a :b :c)   - a proper list, terminated by Nil object
```

### Built-in functions

A function implemented natively. Behaves identically to a `Lambda` from the caller's perspective - first-class, can be passed and stored.

### Lambda functions

An anonymous function object that can be stored in variables, passed as arguments and returned by functions.

Constructed with the `lambda` special form.

Captures the lexical scope in which it was defined (closure).

```lisp
(let add (lambda (x y) (+ x y)))
(add 1 2)  ; 3

; Closures
(let make-adder (lambda (x) (lambda (y) (+ x y))))
(let add5 (make-adder 5))
(add5 3)  ; 8
```

### Exception

Represents a caught runtime error.
The result of an exception catch with `try` special form.

```lisp
(try (car Nil))  ; <WRONG_TYPE>
(try (+ 1 2))    ; Nil - No cought exception
```

## Special Forms, Flow Control and State Mutation

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.
Special forms handle flow control and state mutation.
An incorrect call of a special form raises an `INVALID_SPECIAL_FORM` exception.

### let

`let` introduces a new binding in the current lexical scope.

Usage: `(let name expr)`

Evaluates `expr` and binds the result to name in the current scope.
Returns the bound value.

`name` must be a `symbol`.
Binding a name that is already bound in the current scope raises an `SYMBOL_REBINDING` exception.
shadowing a name from a parent scope is allowed.

```lisp
(let x 42)

(print x)        ; 42

(begin
    (let x 10)
    (print x))   ; 10

(print x)        ; 42
```

### set

`set` updates an existing binding.

Usage: `(set name expr)`

Evaluates expr and updates the existing binding of name to the result. Searches outward
through enclosing scopes. Returns the new value.

Using `set` on an unbound name raises an `SYMBOL_UNDEFINED` exception.

```lisp
(let x 1)
(set x 2)
(print x)  ; 2
```

### if

Conditional expression.

Usage:
* `(if condition then)`
* `(if condition then else)`

Evaluates the `condition` and casts the result to `Bool`. If truthy, evaluates and returns`then` expression. If falsy, evaluates and returns `else` if provided, otherwise returns `Nil`.

```lisp
(if (< 1 2) (print "yes"))   ; yes
(if False (print "no"))      ; Nil
```

### lambda

Creates and returns a `Lambda` object that captures the current scope.

Usage: `(lambda args expr)`
`args` is either:
* a proper list of symbols: `(x y z)` - evaluates to a fixed-arity lambda.
* a symbol: `args` - variadic lambda with no positional arguments.
* an improper list of symbols: `(x y . args)` - fixed amount of positional arguments with a list of variadic arguments.

`expr` is the lambda body, evaluated when the lambda is called.

```lisp
(lambda (x y) (+ x y))
(lambda (x) (lambda (y) (+ x y)))  ; curried

(let square (lambda (x) (* x x)))
(square 5)  ; 25
```

``` lisp
(let drop (lambda (head . tail) tail))
(print (drop 1 2 3 4))  ; 2 3 4
```

### quote

Returns its argument unevaluated.

Usage: `(quote expr)`

Shorthand: `'expr`.

```lisp
(quote x)        ; x       - a symbol, not its value
(quote (1 2 3))  ; (1 2 3) - a list, not a call
'(+ 1 2)         ; (+ 1 2)
```

### begin

Evaluates a sequence of expressions in order in a new lexical scope, returns the value of the last one.

Usage: `(begin expr1 expr2 ...)`

``` lisp
(begin
  (let x 1)
  (let y 2)
  (+ x y))  ; 3
```

### while

Conditional loop. Evaluates `condition` and casts the result to `Bool`. While truthy, evaluates `expr` then re-evaluates `condition`. Returns `Nil`.

Usage: `(while condition expr)`

```lisp
(let i 0)
(while (< i 5) (begin
  (print i)
  (set i (+ i 1))
))
```

### try

Catches exceptions, raised when evaluating an expression.

Usage: `(try exception)`

Evaluates its argument.
If a runtime exception occurs, catches it and returns the `Exception` object, otherwise returns `Nil`.

```lisp
(try (car Nil))  ;  <WRONG_TYPE> - an exception was caught
(try (+ 1 2))    ;  Nil - no exception occurred
```

### and

Short-circuiting logical AND.

Usage: `(and expr1 expr2...)`

Evaluates its arguments left-to-right and casts the result to `Bool`.
As soon as any argument evaluates to `False` it stops the arguments evaluation and returns `False`.
If all arguments evaluated to `True`, returns `True`.
With no arguments, returns True.

```lisp
(and 1 2 3)        ; True
(and 1 Nil 3)      ; False
(and)              ; True
```

### or

Short-circuiting logical OR.

Usage: `(or expr1 expr2...)`

Evaluates arguments left-to-right and casts them to `Bool`.
As soon as any argument evaluates to `True` it stops the arguments evaluation and returns `True`.
If all arguments are evaluated to `False`, returns `False`.
With no arguments, returns `False`.

```lisp
(or Nil NIL 3)     ; True
(or Nil NIL)       ; False
(or)               ; False
```

## Built-in Functions

### List Operations
 
* **`(cons x y)`** - constructs a **Cons cell** with `car = x`, `cdr = y`.
* **`(car pair)`** - returns the `car` of a **Cons cell**.
* **`(cdr pair)`** - returns the `cdr` of a **Cons cell**.
* **`(list expr1 expr2 expr3 ...)`** - constructs a proper list from its arguments. With no arguments, returns `Nil`.
* **`(setcar pair x)`** - updates the `car` of an existing Cons cell in place.
* **`(setcdr pair x)`** - updates the `cdr` of an existing Cons cell in place.

### I/O
 
* **`(show expr)`** - prints its argument in its proper lisp syntax format. Returns `Nil`.
* **`(print fmt arg ...)`** - prints a formatted string. Placeholders `$0`, `$1`, ... are replaced with the corresponding arguments. If a placeholder index has no matching argument, a runtime exception is raised. Returns `Nil`.
 
```lisp
(print "Hello $0!" "World")       ; Hello World!
(print "$0 + $0 = $1" 5 10)       ; 5 + 5 = 10
(print "x = $0, y = $1" 1 2)      ; x = 1, y = 2
```
 
### Evaluation
 
* **`(eval expr)`** - evaluates `expr` as a Sparkle expression in the current scope.
 
```lisp
(eval '(+ 1 2))         ; 3
(eval (list '+ 1 2))    ; 3
 
(let x 10)
(eval 'x)               ; 10
```
 
### Arithmetic

Arithmetic operations work on `Integer` and `Float` values only. Passing any other type is a runtime exception.
If at least one argument is `Float`, the result is `Float`. Otherwise the result is `Integer`.
Any kind of division by zero is a runtime exception.

* `(+ x y ...)` - sum of all arguments. With no arguments, returns `0`.
* `(- x y1 y2 ...)` - difference `x - y1 - y2 ...`.
* `(* x y ...)` - product of all arguments. With no arguments, returns `1`.
* `(/ x y1 y2 ...)` - floating-point division `x / y1 / y2 ...`.
* `(div x y)` - integer division of `x` by  `y`.
* `(mod x y)` - remainder of `x / y`.

### Logic

* **`(not x)`** - casts `x` to `Bool` and returns the opposite value.

### Comparison

#### Equality

`=` and `!=` compare values differently depending on their type:

* `Nil` - all `Nil` are equal.
* `Bool`, `Integer`, `Float`, `String`, `Exception` - comparison by value.
* `Cons`, `Lambda`, `Builtin` - reference - two objects are equal only if they are the same object.

#### Ordering

Ordering operations work on `Integer` and `Float` values only. Passing any other type is a runtime exception.

* **`(> x y)`** - returns `True` if `x` is greater than `y`, otherwise `False`.
* **`(< x y)`** - returns `True` if `x` is less than `y`, otherwise `False`.
* **`(>= x y)`** - returns `True` if `x` is greater than or equal to `y`, otherwise `False`.
* **`(<= x y)`** - returns `True` if `x` is less than or equal to `y`, otherwise `False`.

## Exception Handling
 
Runtime exceptions are raised by the interpreter when an operation cannot proceed.
A runtime exception unwinds the call stack until a try form catches it or the program
terminates with a diagnostic message.
 
Exceptions can be caught with `try`:

Each exception carries a kind identifying the cause:

| Kind                   | Raised when                                       |
|------------------------|---------------------------------------------------|
| `WRONG_TYPE`           | A function receives an argument of the wrong type |
| `WRONG_ARITY`          | A function receives the wrong number of arguments |
| `SYMBOL_UNDEFINED`     | A symbol has no binding in scope                  |
| `SYMBOL_REBINDING`     | A name is bound twice in the same scope           |
| `UNCALLABLE`           | A non-callable object is used as a function       |
| `WRONG_VALUE`          | Division or remainder by zero                     |
| `INVALID_SPECIAL_FORM` | A special form receives invalid arguments         |

Catching exceptions with `try`:

```lisp
(let result (try (car Nil)))
(if (Nil? result) "ok" result)  ; <WRONG_TYPE>
```
When an exception is not caught, the interpreter exits with a non-zero status and prints a human-readable description of the error.

