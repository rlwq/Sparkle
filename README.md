# Sparkle

[![License: MIT](https://img.shields.io/badge/license-MIT-2c7887.svg)](https://opensource.org/licenses/MIT)
![Dialect](https://img.shields.io/badge/dialect-Lisp-2c7887)
![Status](https://img.shields.io/badge/status-active_development-2c7887)
![ISO C11](https://img.shields.io/badge/language-ISO_C11-2c7887)
![Dependencies](https://img.shields.io/badge/dependencies-none-2c7887)

Sparkle is a [Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language)) dialect designed to be intuitive and extensible.
The **AST-walking interpreter** is written from scratch in **ISO C11**, no dependencies required. For a full language reference, see [Sparkle.md](./Sparkle.md).

> [!WARNING]
> This project is currently under active development.
> Bugs, memory leaks, and undefined behavior are to be expected.
> Some features are incomplete, experimental, or in a raw state.

> [!WARNING]
> Not all features described in [Sparkle.md](./Sparkle.md) are yet implemented.
> The language specification is under active development - some described behaviour may differ from the actual interpreter.

## Main features

* [**Homoiconicity**](https://en.wikipedia.org/wiki/Homoiconicity) - code is represented as data, allowing the program to manipulate and modify its own structure at runtime
* [**First-class functions**](https://en.wikipedia.org/wiki/First-class_function) - anonymous functions and functions as objects
* [**Lexical closures**](https://en.wikipedia.org/wiki/Closure_(computer_programming)) - functions capture their lexical environment
* [**Mark-and-sweep GC**](https://en.wikipedia.org/wiki/Tracing_garbage_collection) - automatic memory management
* **Exception handling** - the ability to catch and handle runtime errors.

### To be done

* **REPL** - interactive read-eval-print loop for live code interaction
* **Standard library** - a rich set of general-purpose functions and modules 
* **Modular system** - organize code into reusable, importable modules
* **Macro system** - code transformation before evaluation (Lisp-style macros)

To see what's currently being worked on or what's coming next, check out the [TODO.md](./TODO.md) file.

## Build

```bash
# Optimized build
make build

# Debug build (with ASan, UBSan, Assertions)
make debug
```

## Usage

```bash
./build/sparkle source.rkl
```

## Examples

Sparkle supports both imperative and functional programming styles.

```lisp
(print "Hello, World!")  ; Hello, World!
(print (* 12 13))        ; 156

(let passwd 121213)
(print (+ 1 passwd))     ; 121214
```

Functions are defined using the `lambda` keyword. Sparkle supports functional logic.

```lisp
(let abs (lambda (x) (if (< x 0) (neg x) x)))

(print (abs 15))  ; 15
(print (abs -3))  ; 3
```

Sparkle supports lexical closures, allowing functions to capture their defining environment.

```lisp
(let adder_factory (lambda (x) (lambda (y) (+ x y))))

(let add3 (adder_factory 3))
(let add5 (adder_factory 5))

(print (add3 (add5 7)))  ; 15
```

## Internal Design

### Memory Management

Sparkle uses a **mark-and-sweep** garbage collector that automatically frees unused objects.
When triggered, the GC traverses all reachable objects, marks them, and then frees everything that remains unmarked.

### Scoping

Sparkle uses lexical scoping with chained environments.
Each scope stores symbol–value bindings and may reference a parent scope. 
Variable resolution traverses the scope chain outward until a matching binding is found.
Lambda functions capture the scope in which they are defined, allowing them to access variables from their enclosing environment.

## License

This project is licensed under the [MIT LICENSE](./LICENSE).

