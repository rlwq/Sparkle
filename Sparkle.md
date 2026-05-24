# Sparkle Language Specification

This document defines the semantics of Sparkle as a development & documental reference.

## Program model

* A **program** is a sequence of **s-expressions** evaluated top to bottom. Each expression is evaluated before the next begins.
* Sparkle uses strict evaluation (call-by-value): in a function call, all arguments are
evaluated left-to-right before the function is invoked.
* **Special forms** differ from function calls: they receive their arguments unevaluated and control evaluation themselves. Each special form documents which arguments it evaluates and when.

## Core Types

Sparkle has the following types: `Nil`, `Bool`, `Integer`, `Float`, `String`, `Symbol`, `Cons`, `Lambda` and `Builtin`.

### Nil

Represents the absence of a value.

Literal syntax: `()`.
A symbol with value `Nil` evaluates to the `Nil` object.

Evaluation: self-evaluating - produces the `Nil` object.

A **Proper list** terminator.
Represents an empty **proper list**.

```lisp 
(eval 'Nil)      ; Nil (object of type Nil)
(print "$0" ())  ; Nil
```

### Bool

Represents a boolean value.

A symbol with value `True` evaluates to a `Bool` object with value `True`.
A symbol with value `False` evaluates to a `Bool` object with value `False`.

Evaluation: self-evaluating - produces the corresponding `Bool` object.

Use `(? value)` function to explicitly cast any value to `Bool`.

Truthiness rules for casting:
* `Nil` → always `False`
* `Bool` → itself
* `Integer`, `Float` → `False` if `0`, otherwise `True`
* `String` → `False` if empty, otherwise `True`
* `Cons`, `Lambda`, `Builtin`, `Symbol` → always `True`

```lisp
(? 0)    ; False
(? 1)    ; True
(? Nil)  ; False
(? "")   ; False
```

### Symbol

An object representing an identifier.

Literal syntax: any sequence of non-whitespace, non-parenthesis, non-quote characters that is not a valid `Integer` or `Float` literal. Examples: `x`, `foo`, `nil?`, `+`.

Evaluation:
* `Nil`, `True`, `False` - produce their respective typed values.
* Any other symbol beginning with an uppercase letter - self-evaluating, produces itself as a symbol.
* Any other symbol resolves to the value bound to the name in the current scope. Raises `UNDEFINED_EXCEPTION` if not found.

When unevaluated (e.g. via `quote`), represents itself - a name, not a reference.

```lisp
(let x 42)

x                         ; 42
(quote symbol-is-itself)  ; symbol-is-itself
(eval True)               ; True (a boolean value)
(eval Maybe)              ; Maybe (a symbol)
(= Foo Bar)               ; False
```

### Integer

A signed integer value.

Literal syntax: an optional sign (`+` or `-`) followed by one or more decimal digits. Examples: `42`, `-7`, `+3`.

Evaluation: self-evaluating - an integer value produces itself.

### Float

A floating-point number.

Literal syntax: an optional sign (`+` or `-`), one or more decimal digits, a dot, one or more decimal digits. Examples: `3.14`, `-0.5`, `+1.0`. Examples: `3.14`, `-0.5`.

Evaluation: self-evaluating - a float value produces itself.

### String

An immutable sequence of characters.
Ex.: `"hello"`, `"world"`.

Use `str` to cast a value to a representing `String`.

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

Lists are not a separate type - they are a convention built on nested `Cons` cells.

Evaluation:
Evaluated as a function call or a special form, depending on the `car`.
* If `car` is a symbol naming a special form - the special form is invoked. Arguments are passed unevaluated.
* If `car` evaluates to a `Lambda` or `Builtin` - called as a function. Arguments are evaluated left-to-right before the call.
* If `car` evaluates to anything else - raises `UNCALLABLE_EXCEPTION`.
* `cdr` must be a proper list. An improper list raises `TYPE_EXCEPTION`.

##### Proper lists

A **proper list** is a chain of `Cons` cells terminated by `Nil`.

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
(A B C . Nil)        ; (A B C)      - a proper list, terminated by Nil object
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

## Special Forms, Flow Control and State Mutation

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.
Special forms handle flow control and state mutation.
An incorrect call of a special form raises an `VALUE_EXCEPTION` exception.

### let

`let` introduces a new binding in the current lexical scope.

Usage: `(let name1 expr1 name2 expr2 ...)`

Evaluates each `expr` in order and binds the result to the corresponding `name`.
Each binding is visible to subsequent expressions in the same `let` form. 
An odd number of arguments raises `VALUE_EXCEPTION`.
Returns the last bound value.

Each `name` must be a `symbol`.
Binding a name that is already bound in the current scope raises an `REBINDING_EXCEPTION` exception.
shadowing a name from a parent scope is allowed.

```lisp
(let x 42)              ; 42
(let a 1                ; a = 1
     b (+ a 1))         ; b = 2
(let q 1 p 2)           ; binds q to 1, then p to 2
```

### set

`set` updates an existing bindings in parallel.

Usage: `(set name1 expr1 name2 expr2 ...)`

All `expr`s are evaluated first, in order.
Then all assignments are performed.
Searches outward through enclosing scopes for each name.
Returns the last assigned value.
An odd number of arguments raises `VALUE_EXCEPTION`.
Using `set` on an unbound name raises an `UNDEFINED_EXCEPTION` exception.

```lisp
(let x 17
     y 49)

(set x 2)  ; 2

(set y x          ; y = 2
     x (+ x y))   ; x = 51
```

### if

Conditional expression with multiple branches.

Usage:
* `(if condition1 then1 condition2 then2 ... [default])`

Evaluates `condition`s and casts the result to `Bool` until one of them is `True` and then evaluates and returns the corresponding `then`.
If no condition is `True` and a `default` expression is provided, evaluates and returns it, `Nil` otherwise.

```lisp
(if False (print "no"))     ; Nil

(if True (print "yes"))     ; yes

(if False 1
    False 2
    3)                      ; 3  — default branch

(if False 1
    True  2
    False 3)                ; 2  — second condition matched

(if False 1
    False 2)                ; Nil — no match, no default
```

### lambda

Creates and returns a `Lambda` object that captures the current scope.

Usage: `(lambda args expr1 expr2 ...)`

`args` is either:
* a proper list of symbols: `(x y z)` - evaluates to a fixed-arity lambda.
* a symbol: `args` - variadic lambda with no positional arguments.
* an improper list of symbols: `(x y . args)` - fixed amount of positional arguments with a list of variadic arguments.

The body consists of one or more expressions evaluated in order in a new lexical
scope.
The value of the last expression is returned.
Duplicate argument names raise `VALUE_EXCEPTION`.

```lisp
(let add (lambda (x y) (+ x y)))
(add 1 2)  ; 3

(let verbose-add (lambda (x y)
  (let result (+ x y))
  result))
(verbose-add 3 4)  ; 7
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
Returns `Nil` if no body was provided.

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
  (print "$0" i)
  (set i (+ i 1))
))
```

### try

Catches exceptions, raised when evaluating an expression.

Usage: `(try ExceptionSymbol expr1 expr2...)`

Evaluates `expr1 expr2 ...` as a local lexical scope.
If an exception matching `ExceptionSymbol` is raised, catches it and returns `ExceptionSymbol`.
If a different exception is raised, it propagates normally.
If no exception occurs, returns the value of the last expression.
`ExceptionSymbol` is evaluated, so it must me an self-evaluating symbol or an expression resulting in a symbol.

```lisp
(try TYPE_EXCEPTION (car Nil))   ; TYPE_EXCEPTION — caught
(try TYPE_EXCEPTION (+ 1 2))     ; 3 — no exception
(try TYPE_EXCEPTION (div 1 0))   ; propagates VALUE_EXCEPTION — not caught
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
(or Nil 0 3)       ; True
(or Nil False)     ; False
(or)               ; False
```

## Built-in Functions

### List Operations
 
* **`(cons x y)`** - constructs a **Cons cell** with `car = x`, `cdr = y`.
* **`(car pair)`** - returns the `car` of a **Cons cell**.
* **`(cdr pair)`** - returns the `cdr` of a **Cons cell**.
* **`(list expr1 expr2 expr3 ...)`** - constructs a proper list from its arguments. With no arguments, returns `Nil`.
* **`(len l)`** - length of a proper list `l`.
* **`(setcar pair x)`** - updates the `car` of an existing Cons cell in place.
* **`(setcdr pair x)`** - updates the `cdr` of an existing Cons cell in place.
* **`(map func li)`** - builds a new list by applying `func` to every `li` item.

### I/O
 
* **`(print fmt arg1 arg2 ...)`** - prints a formatted string. Placeholders `$0`, `$1`, ... are replaced with the corresponding arguments. If a placeholder index has no matching argument, a runtime exception is raised. Returns `Nil`.
 
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

Arithmetic operations work on `Bool`, `Integer` and `Float` values only. Passing any other type is a runtime exception.
If at least one argument is `Float`, the result is `Float`. Otherwise the result is `Integer`.
Integer division and modulo by zero are runtime exceptions.

* `(+ x y ...)` - sum of all arguments. With no arguments, returns `0`.
* `(* x y ...)` - product of all arguments. With no arguments, returns `1`.
* `(- x y)` - difference `x - y`.
* `(/ x y)` - floating-point division `x` by `y`.
* `(div x y)` - integer division of `x` by `y`.
* `(mod x y)` - remainder of `x / y`.
* `(neg x) `  - negation. `-x`.

### Logic

* `(? x)` - casts `x` to `Bool`.
* `(not x)` - logical negation of `x`.
* `(|| x1 x2 x3...)` - returns `True` if at least one argument is truthy, otherwise `False`.
* `(&& x1 x2 x3...)` - returns `True` if all arguments are truthy, otherwise `False`.

`||` and `&&` evaluate all arguments before checking. Use `or` and `and` special forms for short-circuiting evaluation.

### Comparison

#### Equality

`=` and `!=` compare values differently depending on their type:

* `Nil` - all `Nil` are equal.
* `Bool`, `Integer`, `Float`, `String`, `Symbol` - comparison by value.
* `Cons`, `Lambda`, `Builtin` - reference - two objects are equal only if they are the same object.
* `Bool`, `Integer`, and `Float` values are comparable with each other. 
* Any other cross-type comparison is a runtime exception.

#### Ordering

Ordering operations work on `Bool`, `Integer` and `Float` values only. Passing any other type is a runtime exception.

* **`(> x y)`** - returns `True` if `x` is greater than `y`, otherwise `False`.
* **`(< x y)`** - returns `True` if `x` is less than `y`, otherwise `False`.
* **`(>= x y)`** - returns `True` if `x` is greater than or equal to `y`, otherwise `False`.
* **`(<= x y)`** - returns `True` if `x` is less than or equal to `y`, otherwise `False`.

## Exception Handling
 
Runtime exceptions are raised by the interpreter when an operation cannot proceed. A runtime exception unwinds the call stack until a try form catches it or the program terminates with a diagnostic message.
 
Exceptions can be caught with `try`:

Each exception carries a kind identifying the cause:

| Symbol                 | Raised when                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| `TYPE_EXCEPTION`       | A function receives an argument of the wrong type                           |
| `ARITY_EXCEPTION`      | A function receives the wrong number of arguments                           |
| `UNDEFINED_EXCEPTION`  | A symbol has no binding in scope                                            |
| `REBINDING_EXCEPTION`  | A name is bound twice in the same scope                                     |
| `UNCALLABLE_EXCEPTION` | A non-callable object is used as a function                                 |
| `VALUE_EXCEPTION`      | A function receives an argument of unexpected value (e.g. division by zero) |

Catching exceptions with `try`:

```lisp
(let result (try TYPE_EXCEPTION (car Nil)))
(print "$0" result)      ; TYPE_EXCEPTION
```

When an exception is not caught, the interpreter exits with a non-zero status and prints a human-readable description of the error.

