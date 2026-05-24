# Sparkle Language Specification

## Syntax

### Literals

Literals are the syntactic representation of fixed values of a specific type.
Each literal parses directly into a value of a corresponding type.

The following definitions are used throughout this section:

```ebnf
decimal-digits = [0-9]+;
binary-digits  = [01]+;
octal-digits   = [0-7]+;
hex-digits     = [0-9a-fA-F]+;

sign = '+' | '-';
```

#### Integer

An integer literal parses into a value of type `Integer`.
Examples: `42`, `-7`, `+3`, `0b1010`, `0o17`, `0xFF`, `-0b101`.

```ebnf
integer      = sign? integer-body;

integer-body = decimal-digits
             | '0b' binary-digits
             | '0o' octal-digits
             | '0x' hex-digits;
```

#### Float

A float literal parses into a value of type `Float`.
Examples: `3.14`, `-0.5`, `.25`, `1.`, `1e10`, `1.5e-3`, `.5E+2`.

```ebnf
float = sign? float-body exponent?
      | sign? decimal-digits exponent;

float-body = decimal-digits '.' decimal-digits?
           | '.' decimal-digits;

exponent = ('e' | 'E') sign? decimal-digits;
```

#### String

A string literal parses into a value of type `String`.
Examples: `"hello"`, `"line1\nline2"`, `"say \"hi\""`.

```ebnf
string = '"' string-character* '"';

string-character = ? any character except '"' and '\' ?
                 | escape-sequence;

escape-sequence = '\\' | '\"' | '\n' | '\r' | '\t';
```

#### Symbol

A symbol literal parses into a value of type `Symbol`.
Examples: `my_variable`, `Hello123`, `>>=`.

```ebnf
symbol = symbol-char+ - (integer | float);

symbol-char = ? any character except whitespace, '(', ')', '\'', '.', '"', ';' ?;
```

> `True`, `False`, `Nil` are syntactically valid symbols and parsed as ones. 

## Types

## Evaluation Model

## Special Forms

## Standard Library

## Exception Model
