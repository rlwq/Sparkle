# Sparkle Language Specification

## Syntax

### Literals

Literals are the syntactic representation of fixed values of a specific type.
Each literal parses directly into a value of a corresponding type.

The following definitions are used throughout this section:

```ebnf
decimal-digit = [0-9];
binary-digit  = [01];
octal-digit   = [0-7];
hex-digit     = [0-9a-fA-F];

decimal-digits = decimal-digit+;
binary-digits  = binary-digit+;
octal-digits   = octal-digit+;
hex-digits     = hex-digit+;

sign = '+' | '-';
```

#### Integer

An integer literal parses into a value of type `Integer`.
Examples: `42`, `-7`, `+3`, `0b1010`, `0o17`, `0xFF`, `-0b101`.

```ebnf
integer      = sign?, integer-body;

integer-body = decimal-digits
             | '0b', binary-digits
             | '0o', octal-digits
             | '0x', hex-digits;
```

#### Float

A float literal parses into a value of type `Float`.
Examples: `3.14`, `-0.5`, `.25`, `1.`, `1e10`, `1.5e-3`, `.5E+2`.

```ebnf
float      = sign?, float-body, exponent?
           | sign?, decimal-digits, exponent;

float-body = decimal-digits, '.', decimal-digits?
           | '.', decimal-digits;

exponent   = ('e' | 'E'), sign?, decimal-digits;
```

#### String

A string literal parses into a value of type `String`.
Examples: `"hello"`, `"line1\nline2"`, `"say \"hi\""`.

```ebnf
string           = '"', string-character*, '"';

string-character = ? any character except '"' and '\\' ?
                 | escape-sequence;

escape-sequence  = '\\\\' | '\\"' | '\\n' | '\\r' | '\\t'
                 | '\\x', 2 * hex-digit;
```

#### Symbol

A symbol literal parses into a value of type `Symbol`.
Examples: `my_variable`, `Hello123`, `>>=`.

```ebnf
symbol      = symbol-char+ - (integer | float);

symbol-char = ? any character except whitespace, '(', ')', '\'', '"', ';' ?;
```

> `True`, `False`, `Nil` are syntactically valid symbols and are parsed as such.

#### S-expression

An s-expression is either a literal or a list.
A list is a parenthesized sequence of s-expressions.

```ebnf
s-expr      = atom
            | list
            | quoted-expr;

list        = '(', list-body, ')';
list-body   = s-expr*;

quoted-expr = '\'', s-expr;

atom        = integer | float | string | symbol;
```

#### Comments

A line comment starts with `;` and runs to the end of the line.
A block comment starts with `/*` and runs, across any number of lines, to the
matching `*/`. Unlike in C, block comments nest: every inner `/*` must be
closed by its own `*/`. Neither marker has any meaning inside a string
literal. A `;` always starts a comment outside strings (it is not a symbol
character), while `/*` adjacent to symbol characters is part of the symbol -
block comments are recognized only between tokens. A block comment left
unterminated at end of input is a lexing error.

```ebnf
line-comment  = ';', ? any character except newline ?*;
block-comment = '/*', (block-comment | ? any text without '/*' or '*/' ?)*, '*/';
```

## Types
 
Sparkle is dynamically typed. Type errors are detected at runtime.
The language defines these types: `Nil`, `Bool`, `Integer`, `Float`, `String`, `Symbol`, `List`, `Lambda`, and `Builtin`.
 
### Nil
 
`Nil` is a unit type with exactly one value.

It represents the absence of a meaningful value.
The symbol `Nil` evaluates to the `Nil` value. `Nil` is distinct from the empty list `()`, which is its own empty `List` value.
`Nil` is falsy.
 
### Bool
 
`Bool` has exactly two values: `True` and `False`.
 
The symbols `True` and `False` evaluate to the corresponding `Bool` values.

`True` is truthy. `False` is falsy.

Any value can be cast to `Bool` via the `?` function. The truthiness rules are:
 
| Type | Falsy when | Truthy when |
|---|---|---|
| `Nil` | always | - |
| `Bool` | `False` | `True` |
| `Integer` | `0` | otherwise |
| `Float` | `0.0` | otherwise |
| `String` | empty | non-empty |
| `List` | empty | non-empty |
| `Symbol`, `Lambda`, `Builtin` | - | always |
 
### Integer
 
`Integer` is a signed integer type.
 
The range of representable values is implementation-defined. The reference implementation uses a 64-bit signed integer (`long long int`).

Arithmetic overflow behavior is implementation-defined.

`0` is falsy. All other `Integer` values are truthy.
 
### Float
 
`Float` is a double-precision floating-point type, conforming to IEEE 754.
 
`0.0` is falsy. All other `Float` values, including `NaN`, are truthy.
 
### Numeric coercion
 
`Bool`, `Integer`, and `Float` form a numeric tower. When an arithmetic or comparison operation receives operands of mixed types, they are coerced to a common type according to the following rules:
 
- If either operand is `Float`, both are coerced to `Float`.
- Otherwise, if either operand is `Integer`, `Bool` is coerced to `Integer`.
Coercion: `False` → `0`, `True` → `1`.
 
Operations on mixed numeric types outside of arithmetic and ordering comparisons raise `TYPE_EXCEPTION`.
 
### String
 
`String` is an immutable sequence of ASCII characters.
 
Two `String` values are equal if and only if they contain the same sequence of characters.

An empty string is falsy. All non-empty strings are truthy.
 
### Symbol
 
`Symbol` is a type representing an identifier. Two symbols with the same character sequence are the same value; symbol identity is determined by name, not by object identity.
 
Evaluation of a symbol depends on its name:
 
- The symbol `Nil` evaluates to the `Nil` value.
- The symbol `True` evaluates to the `Bool` value `True`.
- The symbol `False` evaluates to the `Bool` value `False`.
- Any other symbol whose first character is an uppercase ASCII letter (`A`–`Z`) is *self-evaluating*: it evaluates to itself.
- All other symbols resolve to the value bound to that name in the current scope. If no binding exists, `UNDEFINED_EXCEPTION` is raised.
A `Symbol` value is always truthy.
 
### List
 
`List` is an ordered, dynamically-sized, index-addressable sequence of values. It is represented as a dynamic array; the reference implementation uses a growable array of object references. A `List` is not built from pairs.

Every list is a flat, finite sequence of elements indexed from `0`. The number of elements is its length.

The empty list `()` is its own empty `List` value (length 0), distinct from `Nil`. Both `()` and `(list)` produce an empty `List`. `Nil` does not terminate lists and is not an element of a list unless explicitly stored.
 
Two `List` values are equal if and only if they are the same object (reference equality).

An empty `List` is falsy. A non-empty `List` is truthy.

#### Evaluation as a call
 
The empty list `()` self-evaluates to the empty list.

A non-empty list expression is evaluated as a call or special form, determined by its first element (index 0):

- If the first element is a symbol naming a special form, that special form is invoked and receives the remaining elements unevaluated.
- Otherwise the first element is evaluated. It must yield a `Lambda` or `Builtin`; if it yields any other type, `UNCALLABLE_EXCEPTION` is raised. The remaining elements are then evaluated left-to-right and passed as arguments.
 
### Lambda
 
`Lambda` is a first-class function value. It carries a parameter list, a body expression, and a reference to the lexical scope in which it was defined (a closure).
 
Two `Lambda` values are equal if and only if they are the same object (reference equality).

A `Lambda` value is always truthy.
 
### Builtin
 
`Builtin` is a function implemented natively by the interpreter. From the caller's perspective it is identical to `Lambda`: first-class, callable, passable as a value.
 
Two `Builtin` values are equal if and only if they are the same object (reference equality).

A `Builtin` value is always truthy.

## Evaluation Model

## Special Forms

Special forms look like function calls but are evaluated differently - arguments are not evaluated before being passed and are not bound to any scope - there is no function-like machinery involved.
Special forms handle flow control and state mutation.
An incorrect call of a special form raises a `VALUE_EXCEPTION` exception.

### let

`let` introduces a new binding in the current lexical scope.

Usage: `(let name1 expr1 name2 expr2 ...)`

Evaluates each `expr` in order and binds the result to the corresponding `name`.
Each binding is visible to subsequent expressions in the same `let` form. 
An odd number of arguments raises `VALUE_EXCEPTION`.
Returns the last bound value.

Each `name` must be a `Symbol`.
Binding a name that is already bound in the current scope raises a `REBINDING_EXCEPTION` exception.
Shadowing a name from a parent scope is allowed.

### set

`set` updates existing bindings in parallel.

Usage: `(set name1 expr1 name2 expr2 ...)`

All `expr`s are evaluated first, in order.
Then all assignments are performed.
Searches outward through enclosing scopes for each name.
Returns the last assigned value.
An odd number of arguments raises `VALUE_EXCEPTION`.
Using `set` on an unbound name raises an `UNDEFINED_EXCEPTION` exception.

### if

Conditional expression with multiple branches.

Usage:
* `(if condition1 then1 condition2 then2 ... [default])`

Evaluates `condition`s and casts the result to `Bool` until one of them is `True` and then evaluates and returns the corresponding `then`.
If no condition is `True` and a `default` expression is provided, evaluates and returns it, `Nil` otherwise.

### lambda

Creates and returns a `Lambda` object that captures the current scope.

Usage: `(lambda args expr1 expr2 ...)`

`args` is either:
* a list of symbols: `(x y z)` - evaluates to a fixed-arity lambda.
* a single symbol: `args` - fully variadic lambda with no positional arguments; all arguments are collected into a `List` bound to that symbol.
* a list of symbols containing the reserved marker symbol `Var` as its second-to-last element: `(x y Var rest)` - the symbols before `Var` are fixed positional parameters, and the single symbol after `Var` is bound to a `List` of the remaining arguments.

The body consists of one or more expressions evaluated in order in a new lexical
scope.
The value of the last expression is returned.
Duplicate argument names raise `VALUE_EXCEPTION`.

### quote

Returns its argument unevaluated.

Usage: `(quote expr)`

Shorthand: `'expr`.

### begin

Evaluates a sequence of expressions in order in a new lexical scope, returns the value of the last one.
Returns `Nil` if no body was provided.

Usage: `(begin expr1 expr2 ...)`

### while

Conditional loop. Evaluates `condition` and casts the result to `Bool`. While truthy, evaluates `expr` then re-evaluates `condition`. Returns `Nil`.

Usage: `(while condition expr)`

### try

Catches exceptions raised when evaluating an expression.

Usage: `(try ExceptionSymbol expr1 expr2...)`

Evaluates `expr1 expr2 ...` in a local lexical scope.
If an exception matching `ExceptionSymbol` is raised, catches it and returns `ExceptionSymbol`.
If a different exception is raised, it propagates normally.
If no exception occurs, returns the value of the last expression.
`ExceptionSymbol` is evaluated, so it must be a self-evaluating symbol or an expression resulting in a symbol.

### and

Short-circuiting logical AND.

Usage: `(and expr1 expr2...)`

Evaluates its arguments left-to-right and casts the result to `Bool`.
As soon as any argument evaluates to `False` it stops evaluating the arguments and returns `False`.
If all arguments evaluate to `True`, returns `True`.
With no arguments, returns `True`.

### or

Short-circuiting logical OR.

Usage: `(or expr1 expr2...)`

Evaluates arguments left-to-right and casts them to `Bool`.
As soon as any argument evaluates to `True` it stops evaluating the arguments and returns `True`.
If all arguments evaluate to `False`, returns `False`.
With no arguments, returns `False`.

## Built-in Functions

### List operations

The following built-ins operate on `List` values. Passing a non-`List` value where a `List` is expected raises `TYPE_EXCEPTION`.

* `(list e1 e2 ...)` - constructs a `List` from the evaluated arguments. With no arguments, returns an empty `List`.
* `(len l)` - returns the length of `List` `l` as an `Integer`.
* `(get l i)` - returns the element of `l` at 0-based index `i`. An out-of-range index raises `VALUE_EXCEPTION`.
* `(put l i x)` - sets the element of `l` at 0-based index `i` to `x` in place and returns `l`. An out-of-range index raises `VALUE_EXCEPTION`.
* `(push l x)` - appends `x` to the end of `l` in place and returns `l`.
* `(pop l)` - removes and returns the last element of `l`. An empty list raises `VALUE_EXCEPTION`.
* `(append l1 l2)` - returns a new `List` containing the elements of `l1` followed by the elements of `l2`.
* `(map func l)` - returns a new `List` obtained by applying `func` to each element of `l`.
* `(filter func l)` - returns a new `List` containing the elements of `l` for which `func` returns a truthy value.

### String operations

The following built-ins operate on `String` values. Passing a non-`String` value where a `String` is expected raises `TYPE_EXCEPTION`.
`String` values are immutable: these functions never modify their arguments and always return new values.

* `(str x)` - returns the printed representation of `x` (any type, as `print` would show it) as a `String`. A `String` argument is returned unchanged.
* `(str-len s)` - returns the length of `String` `s` as an `Integer`.
* `(str-get s i)` - returns the character of `s` at 0-based index `i` as a `String` of length 1. An out-of-range index raises `VALUE_EXCEPTION`.
* `(str-sub s start len)` - returns the substring of `s` of length `len` starting at 0-based index `start`. A negative or out-of-range `start` or `len` raises `VALUE_EXCEPTION`.
* `(str-cat s1 s2 ...)` - returns the concatenation of the evaluated arguments. With no arguments, returns an empty `String`.
* `(str-find s sub)` - returns the 0-based index of the first occurrence of `sub` in `s` as an `Integer`, or `-1` if there is none. An empty `sub` is found at index `0`.
* `(str-ord s)` - returns the character code of the first character of `s` as an `Integer`. An empty `s` raises `VALUE_EXCEPTION`.
* `(str-chr code)` - returns a `String` of length 1 containing the character with code `code`. A `code` outside the ASCII range (`0`-`127`) raises `VALUE_EXCEPTION`.

### I/O

* `(print fmt arg1 arg2 ...)` - writes `fmt` to standard output followed by a newline and returns `Nil`. A non-`String` `fmt` raises `TYPE_EXCEPTION`.

Each placeholder `$N` in `fmt` is replaced by the argument at 0-based position `N` among the arguments following `fmt`, rendered as `str` renders it.
`N` is read as a whole decimal number, so `$10` denotes position `10` rather than `$1` followed by a `0`.
A placeholder may occur any number of times and in any order, and arguments no placeholder refers to are ignored.
A placeholder whose index is not less than the number of arguments following `fmt` raises `VALUE_EXCEPTION`.

`$$` produces a literal `$`. A `$` followed by neither `$` nor a digit stands for itself.

## Standard Library

## Exception Model
