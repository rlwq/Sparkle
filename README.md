# Sparkle

[![License: MIT](https://img.shields.io/badge/license-MIT-2c7887.svg)](https://opensource.org/licenses/MIT)
![Dialect](https://img.shields.io/badge/dialect-Lisp-2c7887)
![Status](https://img.shields.io/badge/status-active_development-2c7887)
![ISO C11](https://img.shields.io/badge/language-ISO_C11-2c7887)
![Dependencies](https://img.shields.io/badge/dependencies-none-2c7887)

> [!WARNING]
> This project is currently under active development.
> Bugs, memory leaks, and undefined behavior are to be expected.
> Some features are incomplete, experimental, or in a raw state.

> [!WARNING]
> Not all features described in [Specification.md](./Specification.md) are yet implemented.
> The specification is under active development - some described behavior may differ from the actual interpreter.

## Philosophy

Sparkle is a [Lisp-1](https://en.wikipedia.org/wiki/Lisp_(programming_language))
dialect: Lisp rethought as a modern scripting language. It keeps what makes
Lisp different - code as data, one syntax for everything - and takes the rest
from modern scripting languages: their paradigms, patterns, and conventions.

For the full language reference, see [Specification.md](./Specification.md).

## Main features

* [**Homoiconicity**](https://en.wikipedia.org/wiki/Homoiconicity) - code is represented as data, allowing the program to manipulate and modify its own structure at runtime
* [**First-class functions**](https://en.wikipedia.org/wiki/First-class_function) - anonymous functions and functions as objects
* [**Lexical closures**](https://en.wikipedia.org/wiki/Closure_(computer_programming)) - functions capture their lexical environment
* [**Mark-and-sweep GC**](https://en.wikipedia.org/wiki/Tracing_garbage_collection) - automatic memory management
* **Exception handling** - the ability to catch and handle runtime errors

### To be done

* **REPL** - interactive read-eval-print loop for live code interaction
* **Standard library** - a rich set of general-purpose functions and modules 
* **Module system** - organize code into reusable, importable modules
* **Macro system** - code transformation before evaluation (Lisp-style macros)

To see what's currently being worked on or what's coming next, check out the [TODO.md](./TODO.md) file.

## Build

```bash
# List all available targets
make help    

# Optimized build (O3)
make build

# Debug build (with ASan, UBSan, Assertions)
make debug

# Run the test suite
make test
```

## Usage

```bash
./build/sparkle source.spk
```

## Examples

Sparkle supports both imperative and functional programming styles.

```lisp
(print "Hello, World!")  ; Hello, World!
(print (* 12 13))        ; 156

(let passwd 121213)
(print (+ 1 passwd))     ; 121214
```

Loops and mutation work the imperative way:

```lisp
(let i 0)
(let sum 0)

(while (< i 5)
  (begin
    (set sum (+ sum i))
    (set i (+ i 1))))

(print sum)  ; 10
```

Functions are values, defined with `lambda`:

```lisp
(let abs (lambda (x) (if (< x 0) (- 0 x) x)))

(print (abs 15))  ; 15
(print (abs -3))  ; 3
```

And closures capture their defining environment:

```lisp
(let adder_factory (lambda (x) (lambda (y) (+ x y))))

(let add3 (adder_factory 3))
(let add5 (adder_factory 5))

(print (add3 (add5 7)))  ; 15
```

## Language Design

The language is built to meet the expectations of a modern scripting language:
dynamic typing, automatic memory management, lexical scoping with closures.
What stays Lisp is the syntax and code as data; the rest works the way you
would expect coming from Python or Lua.

### Values and Types

Dynamically typed: types belong to values, not variables. The types are `Nil`,
`Bool`, `Integer`, `Float`, `String`, `Symbol`, `List`, `Lambda`, and
`Builtin`. Strings are immutable; the list is the one mutable container. A
type error is not a crash - it is a `TYPE_EXCEPTION` you can catch.

### Symbols

A concept Lisp had long before everyone else: a symbol is a label that exists
as a value of its own - an enum member without declaring the enum. `Red`,
`Ok`, `NotFound` are themselves, not strings or variables. At the same time
symbols are what values get bound to, just like in any other language: a
variable name is simply a symbol with a value behind it.

### Scoping and Closures

Lexical scoping with chained environments: resolution walks the scope chain
outward until it finds the binding. `let` defines in the current scope, `set`
mutates an existing binding. A lambda captures the scope it was defined in and
carries it along - closures fall out of that for free.

### Special Forms

Special forms look like function calls but are not: they receive their
arguments unevaluated. Ordinary functions can't do that, and that is exactly
the logic control flow and state mutation are made of - `if` must not
evaluate both branches, `let` must not evaluate the name it binds. Everything
that doesn't need this is just a function.

### Exceptions and Exception Handling

Runtime errors are exceptions, like in any modern scripting language: a type
error or a wrong arity raises, the stack unwinds to the nearest handler, and
`try` catches the exception and hands it to you as a value to inspect.

## Interpreter Design

The interpreter is an **AST-walker** written from scratch in **ISO C11** with
zero dependencies.

### Pipeline

Three stages, no bytecode: a **lexer** turns the source into tokens, a
**recursive-descent parser** turns tokens into expression trees, and the
**VM** walks those trees directly.

### Stacks and Roots

Every intermediate value lives on an explicit value stack, every live scope on
a scope stack. This is more than bookkeeping: together with the not-yet-run
top-level expressions, the stacks form the GC's root set - if a value
matters, it is rooted, so the collector always knows what is alive.

### Builtins and Special Forms

The machine knows nothing about the language running on it. Builtin functions
are registered into the global scope from declarative tables; special-form
dispatch is injected into the VM as a hook. Extending the language means
adding a table entry, not touching the core.

### Memory Management

A **mark-and-sweep** garbage collector owns every object from the moment it is
allocated. Collection happens only at safe points inside allocation, where
everything alive is guaranteed to be rooted on the stacks: mark what is
reachable, sweep the rest.

### Error Recovery

Exceptions unwind with `setjmp`/`longjmp`: raising an error jumps to the
nearest recovery point and truncates the stacks back to the depth recorded
there - no error codes threaded through the interpreter.

## Use of AI

AI took no part in making decisions - the architecture, the code conventions,
the specification, and the philosophy of the language are all human work.

AI was used strictly to generate code on top of the already-designed core,
architecture, and conventions: implementing the many small pieces over the
ready base, following the [spec](./Specification.md). Beyond that - only
refactoring and finding bugs, which a human then worked through.

## License

This project is licensed under the [MIT LICENSE](./LICENSE).

