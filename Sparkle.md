# Sparkle Language Specification

This document defines the semantics of Sparkle as a development & documental reference.

## Program model

* A **program** is a sequence of **s-expressions** evaluated top to bottom. Each expression is evaluated before the next begins.
* Sparkle uses strict evaluation (call-by-value): in a function call, all arguments are
evaluated left-to-right before the function is invoked.
* **Special forms** differ from function calls: they receive their arguments unevaluated and control evaluation themselves. Each special form documents which arguments it evaluates and when.

## Core Types

Sparkle has the following types: `Nil`, `Bool`, `Integer`, `Float`, `String`, `Symbol`, `List`, `Lambda` and `Builtin`.

### Nil

Represents the absence of a value.

A symbol with value `Nil` evaluates to the `Nil` object.

Evaluation: self-evaluating - produces the `Nil` object.

`Nil` is purely the absence of a value. It is distinct from the empty list `()` and does not serve as a list terminator.

```lisp 
(eval 'Nil)         ; Nil (object of type Nil)
(print "$0" Nil)    ; Nil
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
* `List` → `False` if empty, otherwise `True`
* `Lambda`, `Builtin`, `Symbol` → always `True`

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

### List

A **List** is an ordered, dynamically-sized sequence of values, backed by a dynamic array (not nested pairs).

Literal syntax: `(a b c)` - elements enclosed in parentheses. There is no dotted-pair syntax; the dot `.` is an ordinary symbol character.

The empty list `()` is its own empty `List` value (size 0). It is distinct from `Nil`. Both `()` and `(list)` produce an empty List.

Every list is a flat sequence of elements - there are no "proper" or "improper" lists.

Truthiness: an empty List is falsy; a non-empty List is truthy (mirrors `String`).

Equality (`=` / `!=`): reference equality - two Lists are equal only if they are the same object.

Elements are accessed by 0-based index with `get` and `put`. Build and grow lists with `list`, `push`, `pop` and `append`, and query the size with `len`.

```lisp
(list 1 2 3)          ; (1 2 3)
'(1 2 3)              ; (1 2 3)
()                    ; ()  - the empty list
(get '(1 2 3) 0)      ; 1
(len '(1 2 3))        ; 3
```

Evaluation:
* The empty list `()` self-evaluates to the empty list.
* A non-empty list is evaluated as a function call or a special form, determined by its first element (index 0):
  * If the first element is a symbol naming a special form - the special form is invoked. The remaining elements are passed unevaluated.
  * Otherwise the first element is evaluated and must be a `Lambda` or `Builtin`. The remaining elements are evaluated left-to-right and passed as arguments.
  * If the first element evaluates to anything else - raises `UNCALLABLE_EXCEPTION`.

A List prints as `(elem1 elem2 ...)`; the empty list prints as `()`.

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
* a list of symbols: `(x y z)` - evaluates to a fixed-arity lambda.
* a single symbol: `args` - fully variadic lambda; all arguments are collected into a `List` bound to that symbol.
* a list of symbols containing the `Var` marker: `(x y Var rest)` - `x` and `y` are fixed positional parameters; the symbol immediately following the `Var` marker (`rest`) is bound to a `List` of all remaining arguments.

The body consists of one or more expressions evaluated in order in a new lexical
scope.
The value of the last expression is returned.
Duplicate argument names raise `VALUE_EXCEPTION`.

```lisp
(let add (lambda (x y) (+ x y)))
(add 1 2)  ; 3

; Fully variadic - all arguments collected into a List
(let collect (lambda args args))
(collect 1 2 3)  ; (1 2 3)

; Fixed positional params plus a variadic tail
(let f (lambda (x y Var rest) rest))
(f 1 2 3 4 5)  ; (3 4 5)

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
(try TYPE_EXCEPTION (get Nil 0))  ; TYPE_EXCEPTION — caught
(try TYPE_EXCEPTION (+ 1 2))      ; 3 — no exception
(try TYPE_EXCEPTION (div 1 0))    ; propagates VALUE_EXCEPTION — not caught
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

All of these operate on `List` values and raise `TYPE_EXCEPTION` if given a non-List where a List is expected.

* **`(list e1 e2 ...)`** - constructs a `List` from its arguments. With no arguments, returns the empty List.
* **`(len l)`** - number of elements in list `l`.
* **`(get l i)`** - returns the element at 0-based index `i`. An out-of-range index raises `VALUE_EXCEPTION`.
* **`(put l i x)`** - sets the element at 0-based index `i` to `x`, in place; returns the list. An out-of-range index raises `VALUE_EXCEPTION`.
* **`(push l x)`** - appends `x` to the end of list `l` in place; returns the list.
* **`(pop l)`** - removes and returns the last element of `l`. Popping an empty list raises `VALUE_EXCEPTION`.
* **`(append l1 l2)`** - returns a new List containing the elements of `l1` followed by the elements of `l2`.
* **`(map func l)`** - returns a new List built by applying `func` to every element of `l`.
* **`(filter func l)`** - returns a new List of the elements of `l` for which `func` returns truthy.

```lisp
(list 1 2 3)                    ; (1 2 3)
(list)                          ; ()
(len '(1 2 3))                  ; 3
(get '(a b c) 1)               ; b
(put (list 1 2 3) 0 9)          ; (9 2 3)
(push (list 1 2) 3)             ; (1 2 3)
(pop (list 1 2 3))             ; 3
(append '(1 2) '(3 4))         ; (1 2 3 4)
(map (lambda (x) (* x x)) '(1 2 3))       ; (1 4 9)
(filter (lambda (x) (> x 1)) '(1 2 3))    ; (2 3)
```

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
* `List`, `Lambda`, `Builtin` - reference - two objects are equal only if they are the same object.
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
(let result (try TYPE_EXCEPTION (get Nil 0)))
(print "$0" result)      ; TYPE_EXCEPTION
```

When an exception is not caught, the interpreter exits with a non-zero status and prints a human-readable description of the error.

