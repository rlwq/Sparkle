# Sparkle

Sparkle is a [Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language)) dialect designed to be intuitive and easy to use and reuse.
The **AST-walking interpreter** is written from scratch in **ISO C11**, no dependencies required.

> [!WARNING]
> This project is currently under active development.
> Bugs, memory leaks, and undefined behavior are to be expected.
> Some features are incomplete, experimental, or in a raw state.

## Main features

* [**Homoiconicity**](https://en.wikipedia.org/wiki/Homoiconicity) - code is represented as data, allowing the program to manipulate and modify its own structure at runtime
* [**First-class functions**](https://en.wikipedia.org/wiki/First-class_function) - anonymous functions and functions as objects
* [**Lexical closures**](https://en.wikipedia.org/wiki/Closure_(computer_programming)) - functions capture their lexical environment
* [**Mark-and-sweep GC**](https://en.wikipedia.org/wiki/Tracing_garbage_collection) - automatic memory management (in progress of integration)

### To be done

* **REPL** - interactive read-eval-print loop for live code interaction
* **Standard library** - a rich set of general-purpose functions and modules 
* **Modular system** - organize code into reusable, importable modules
* **Error handling** - descriptive runtime errors with stack traces
* [**Tail call optimization**](https://en.wikipedia.org/wiki/Tail_call) - efficient recursion without stack overflow
* **Macro system** - code transformation before evaluation (Lisp-style macros)

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

## Internal Design

### Memory Management

Sparkle uses a **mark-and-sweep** garbage collector that automatically frees unused objects.
When triggered, the GC traverses all reachable objects, marks them, and then frees everything that remains unmarked.

### Scoping

Sparkle uses lexical scoping with chained environments.
Each scope stores symbol–value bindings and may reference a parent scope. 
Variable lookup walks the scope chain outward until a matching binding is found.
Lambda functions capture the scope in which they are defined, allowing them to access variables from their enclosing environment.

## License

This project is licensed under the [MIT LICENSE](./LICENSE).
