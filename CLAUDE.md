# Sparkle

A Lisp-1 dialect with an AST-walking interpreter in ISO C11, zero dependencies.
Language reference: `Sparkle.md`; language spec: `Specification.md`.

The codebase invariants are documented where they are enforced - read the
comment at the point of use before changing anything nearby:

- `gc.h` - ownership (the GC owns every Object and Scope) and the collection
  protocol: `gc_alloc_*` never collect, only the VM does, at safe points
- `vm.h` - the rooting rule for constructors (build first, pop after) and the
  singleton identity rule
- `object.h` - the checklist for adding a new object kind; string immutability
  and buffer conventions
- `builtins.h` - the value-stack protocol builtins must follow
- `io.h` - `print` and `str` share one representation via `write_expr`

## Commands

- `make build` — release build (`-O3`, LTO, `NDEBUG`) to `./build/sparkle`
- `make debug` — ASan + UBSan build (same output path)
- `make test` — run `tests/positive` and `tests/negative` against `./build/sparkle`
- `make rewrite_tests` — regenerate expected outputs (verify behavior first)
- `make format` — clang-format all sources

Before calling work done: `make build && make test`, then `make debug && make
test` — the sanitizer run is the memory-correctness check.

Gotchas with no code anchor: release defines `NDEBUG`, so a variable used only
in an `assert` needs `(void)x;` to survive `-Werror`; the tester compares
whitespace-normalized stdout (`.out`) and, for negative tests, stderr (`.err`)
plus a non-zero exit code.

## Layout

- `src/sparkle_core/` — lexer, parser, GC, string interner (no VM knowledge)
- `src/vm/` — value/scope stacks, eval, object building, error recovery
- `src/sparkle_impl/` — builtins (`rkl_*`), special forms, printing
- `include/` — flat headers; `tests/` — `.rkl` + expected `.out`/`.err`
