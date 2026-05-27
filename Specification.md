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
                 | '\\x', 2 * hex-digit
                 | '\\u', 4 * hex-digit;
```

#### Symbol

A symbol literal parses into a value of type `Symbol`.
Examples: `my_variable`, `Hello123`, `>>=`.

```ebnf
symbol      = symbol-char+ - (integer | float);

symbol-char = ? any character except whitespace, '(', ')', '\'', '.', '"', ';' ?;
```

> `True`, `False`, `Nil` are syntactically valid symbols and parsed as ones. 

#### S-expression

An s-expression is either a literal or a list.
A list is a parenthesized sequence of s-expressions, optionally terminated by a dotted tail.

```ebnf
s-expr      = atom
            | list
            | quoted-expr;

list        = '(', list-body, ')';
list-body   = s-expr*, ['.', s-expr];

quoted-expr = '\'', s-expr;

atom        = integer | float | string | symbol;
```

## Types
 
Sparkle is dynamically typed. Type errors are detected at runtime.
The language defines these types: `Nil`, `Bool`, `Integer`, `Float`, `String`, `Symbol`, `Cons`, `Lambda`, and `Builtin`.
 
### Nil
 
`Nil` is a unit type with exactly one value.

It represents the absence of a meaningful value and serves as the terminator of proper lists.
The symbol `Nil` and the empty list literal `()` both evaluate to the `Nil` value.
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
| `Symbol`, `Cons`, `Lambda`, `Builtin` | - | always |
 
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
 
`String` is an immutable sequence of characters.
 
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
 
### Cons
 
`Cons` is an ordered pair of two values, referred to as `car` and `cdr`.
 
Two `Cons` values are equal if and only if they are the same object (reference equality).
 
#### Lists
 
Lists are a convention built on nested `Cons` cells.
 
A **proper list** is a `Cons` chain whose last `cdr` is `Nil`. The `Nil` value itself represents the empty proper list.

An **improper list** is a `Cons` chain whose last `cdr` is not `Nil`.

When a `Cons` value is evaluated as the head of a call, the `cdr` must be a proper list; an improper list raises `TYPE_EXCEPTION`.

A `Cons` value is always truthy.
 
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

## Standard Library

## Exception Model
