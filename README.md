# Sparkle

Sparkle is a [Lisp](https://en.wikipedia.org/wiki/Lisp) dialect designed to be intuitive and easy to use and reuse.
The **AST-walking interpreter** is written from scratch in **ISO C11**, no dependencies required.

> [!WARNING]
> This project is currently under active development.
> Bugs, memory leaks, and undefined behavior are to be expected.
> Some features are incomplete, experimental, or in a raw state.

## Main features

* [**Homoiconicity**](https://en.wikipedia.org/wiki/Homoiconicity]) - code is represented as data, allowing the program to manipulate and modify its own structure at runtime
* [**First-class functions**](https://en.wikipedia.org/wiki/First-class_function) - anonymous functions and functions as objects
* [**Lexical closures**](https://en.wikipedia.org/wiki/Closure_(computer_programming)) - functions capture their lexical environment

### To be done

* [**Mark-and-sweep GC**](https://en.wikipedia.org/wiki/Tracing_garbage_collection) - automatic memory management (in progress integration)
* **REPL** - interactive read-eval-print loop for live code interaction
* **Standard library** - a rich set of general-purpose functions and modules 
* **Modular system** - organize code into reusable, importable modules
* **Error handling** - descriptive runtime errors with stack traces
* [**Tail call optimization**](https://en.wikipedia.org/wiki/Tail_call) - efficient recursion without stack overflow


## Build

```bash
# Optimized build
make build

# Debug build (with AddressSanitizer, UBSan, Assertions)
make debug
```

## Usage

```bash
./build/sparkle source.rkl
```

## License

This project is licensed under the [MIT LICENSE](./LICENSE).
