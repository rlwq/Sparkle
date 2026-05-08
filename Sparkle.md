# Sparkle Language Specification

This document defines the semantics of Sparkle as a development & documental reference.

## Program model

* A **program** is a sequence of **s-expressions**.
* Expressions are evaluated in order, top to bottom.
* Sparkle uses strict evaluation (call-by-value): in ordinary function calls, arguments are evaluated before the call.
* Argument evaluation order is left-to-right.

## Core Types

Sparkle has the following types:

* `Nil`
* `Bool`
* `Integer`
* `Float`
* `String`
* `Symbol`
* `Cons` & `Lists`
* `Lambda`
* `Builtin`
* `Exception`

### Nil

Represents the absence of a value or an empty list.
Defined as `Nil` or `()`.
**Proper list** terminator (the empty list is `Nil` itself). 

### Bool

Represents a boolean value. `True` or `False`.
Use `(? value)` to explicitly cast any value to `Bool`.

* `Nil` → `False`
* `Integer`, `Float` → `False` if `0`, otherwise `True`
* `String` → `False` if empty, otherwise `True`
* `Cons`, `Lambda`, `Builtin`, `Symbol`, `Exception` → `True`

```lisp
(? 0)    ; False
(? 1)    ; True
(? Nil)  ; False
(? "")   ; False
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

A signed integer value.
Ex.: `42`, `-7`, `+3`.

### Float

A floating-point number.
Ex.: `3.14`, `-0.5`.

### String

An immutable sequence of characters.
Ex.: `"hello"`, `"world"`.

### Cons & Lists

#### Cons pairs

A **Cons Cell** is an ordered pair of two values.
Its "left" and "right" slots are named `car` and `cdr` respectively.

* Use the `cons` function to build a new cell. 
* Use the `car` and `cdr` functions to retrieve values from the slots.
* Use `setcar` and `setcdr` to update the values within an existing cell.

* `(x . y)` A cell where `car` is `x` and `cdr` is `y`.
* `(x .  )` Evaluates as `(x . Nil)`.
* `(  . x)` Evaluates as `(Nil . x)`.

```lisp
(cons 1 2)        ; a pair
(car (cons 1 2))  ; 1
(cdr (cons 1 2))  ; 2
```

#### Lists

##### Proper lists

Lists are not a separate type - they are a convention built on nested `Cons` cells.
A **proper list** is a chain of Cons cells terminated by `Nil`.

A sequence of values enclosed in parentheses defines a proper list. In this form, the interpreter automatically terminates the chain with Nil.
* **Example**: `(a b c d e)` defines a Nil-terminated list.

```lisp
; Lists are nested cons cells terminated by Nil
(cons 1 (cons 2 (cons 3 Nil)))  ; (1 2 3)

(car '(1 2 3 4 5))              ; 1
(cdr '(1 2 3 4 5))              ; (2 3 4 5)
```

##### Improper lists

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

An anonymous function object that can be stored in variables, passed as arguments and returned by functions.

```lisp
(let add (lambda (x y) (+ x y)))
(add 1 2)  ; 3

; Closures
(let make-adder (lambda (x) (lambda (y) (+ x y))))
(let add5 (make-adder 5))
(add5 3)  ; 8
```

## Special Forms & Flow Control

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.
Special forms handle flow control and state mutation.

### let

`let` introduces a new binding in the current lexical scope.
Allows shadowing of variables defined in parent-scopes.
Rebinding an already bound symbol is a runtime exception.

```lisp
(let x 42)

(print x)        ; 42

(begin
    (let x 10)
    (print x)    ; 10
)

(print x)        ; 42
```

### set

`set` updates an existing binding.
Updates the binding in the local-most scope.
Using set on an unbound name is a runtime exception.

```lisp
(let x 1)
(set x 2)
(print x)  ; 2
```

### if

Conditional expression.
Evaluates the condition, then either the true or false branch.
The else branch is optional - if omitted and the condition is false, returns `Nil`.
Automatically casts the condition to `Bool`.

```lisp
(if (= x 0)
    "zero"
    "non-zero")

(if (< 1 2) (print "Hi!"))  ; Hi!
```

### lambda

Creates a Lambda object capturing the current scope.

```lisp
(lambda (x y) (+ x y))
(lambda (x) (lambda (y) (+ x y)))  ; curried

(let square (lambda (x) (* x x)))
(square 5)  ; 25
```

Can define variadic Lambdas.

``` lisp
(let drop (lambda (head . tail) tail))
(print (drop 1 2 3 4))  ; 2 3 4
```

### quote

Returns its argument unevaluated.
Shorthand: `'expr`.

```lisp
(quote x)        ; x       - a symbol, not its value
(quote (1 2 3))  ; (1 2 3) - a list, not a call
```


### begin

Evaluates a sequence of expressions in order, returns the value of the last one.
Creates a lexical scope.

``` lisp
(begin
  (let x 1)
  (let y 2)
  (+ x y))  ; 3
```

### while

Evaluates the body repeatedly as long as the condition evaluates to `True`.
Returns `Nil`.

```lisp
(let i 0)
(while (< i 5)
  (print i)
  (set i (+ i 1))
)
```

### try

Evaluates its argument.
If a runtime exception occurs, catches it and returns the `Exception` object;
otherwise returns `Nil`.

```lisp
(try (car Nil))  ;  <WRONG_TYPE> - an exception was caught
(try (+ 1 2))    ;  Nil - no exception occurred
```

### and

Evaluates arguments left-to-right and casts the result to `Bool`.
As soon as any argument evaluates to `False` it stops the arguments evaluation and returns `False`.
If all arguments evaluated to `True`, returns `True`.
With no arguments, returns True.

```lisp
(and 1 2 3)        ; True
(and 1 Nil 3)      ; False
(and)              ; True
```

### or

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

* **`(+ x y ...)`** - sum of all arguments. With no arguments, returns `0`.
* **`(- x y1 y2 ...)`** - difference `x - y1 - y2 ...`.
* **`(* x y ...)`** - product of all arguments. With no arguments, returns `1`.
* **`(/ x y1 y2 ...)`** - floating-point division `x / y1 / y2 ...`.
* **`(div x y)`** - integer division of `x` by  `y`.
* **`(mod x y)`** - remainder of `x / y`.
 
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
* **`(not x)`** - casts `x` to `Bool` and returns the opposite value.

## Exception Handling
 
Runtime exceptions are raised by the interpreter when an operation cannot proceed.
Each exception carries an `Exception` object describing what went wrong.
 
Exceptions can be caught with `try`:
 
```lisp
(let result (try (car Nil)))
(if (nil? result)
    (show "ok")
    (show "exception caught"))
```

### Exception categories
 
| Kind                | Raised when                                      |
|---------------------|--------------------------------------------------|
| `WRONG_TYPE`        | A function receives an argument of the wrong type |
| `WRONG_ARITY`       | A function receives the wrong number of arguments |
| `SYMBOL_UNDEFINED`  | A symbol has no binding in scope                 |
| `SYMBOL_REBINDING`  | A name is bound twice in the same scope          |
| `UNCALLABLE`        | A non-callable object is used as a function      |
| `WRONG_VALUE`       | Division or remainder by zero                    |
| `IMPROPER_FORM` | |

