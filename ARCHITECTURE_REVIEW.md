# Architecture Review — Sparkle

Date: 2026-07-19 | Scope: full repository (`src/`, `include/`, `tests/tester.py` glanced for conventions; `build/`, docs excluded) | Language: ISO C11

**Status: all findings (R1–R7) applied and verified on 2026-07-19** — 29 tests
pass under both the release and the ASan/UBSan build; every Verify grep below
returns empty.

## Summary

For a ~3.3k-line hand-written interpreter the architecture is in good shape: the
lexer → parser → object graph → VM → builtins pipeline is cleanly separated,
ownership is centralized in the GC with a documented collection protocol, error
handling is one consistent longjmp-based recovery mechanism, and dispatch over
the closed `ObjectKind` set is centralized one-switch-per-operation and
compiler-enforced. The dominant real problem is not structure but a missing
primitive: the VM has no way to *call* a callable with already-evaluated
arguments, so `map`/`filter` fake it by synthesizing a source expression and
re-entering `vm_eval_node`, which double-evaluates list elements and makes
`map` observably wrong on compound values (confirmed by test). Second: two
upward dependencies from `src/vm/` into `src/sparkle_impl/` invert the layering
the project itself declares. Fix R1 first — it is a user-visible semantics bug
and the `vm_call` primitive it introduces is the natural foundation for future
`apply`/FFI work.

## Findings

### Critical

#### R1: Add a `vm_call` primitive; stop `map`/`filter` re-entering eval with synthetic expressions

- **Severity**: Critical
- **Principle**: LSP (builtins break the "arguments are evaluated exactly once"
  convention) / missing abstraction
- **Location**: `src/sparkle_impl/builtins_list.c` — `rkl_map` (94–111),
  `rkl_filter` (113–137); `src/vm/vm_eval.c` — `eval_lambda_call` (12–60),
  `eval_builtin_call` (63–89), `vm_eval_list` (184–228)
- **Problem**: `rkl_map` packs `(func item)` where `item` is an already-evaluated
  value, then calls `vm_eval_node`, whose call path re-evaluates every argument.
  Any element that is not self-evaluating is therefore evaluated twice:
  `(map (lambda (x) x) (list (list 1 2)))` dies with `UNCALLABLE_EXCEPTION`
  because `(1 2)` is re-evaluated as a call (reproduced on the current build);
  a symbol element would resolve against the current scope instead of being
  passed through. The root cause is that argument evaluation and callable
  invocation are fused inside `eval_lambda_call`/`eval_builtin_call` (both also
  duplicate the same eval-args loop, lines 20–25 and 71–76), so no code path
  can call a function with values it already holds.
- **Change**:
  1. In `vm_eval.c`, extract the argument-evaluation loop shared by
     `eval_lambda_call`/`eval_builtin_call` into the caller: in `vm_eval_list`,
     after the special-form dispatch miss and head evaluation, evaluate the
     packed args list in place (push each element, `vm_eval_node`, repack).
  2. Define `void vm_call(VM *vm)` in `vm_eval.c` with stack contract
     `List (evaluated args), Callable -> Node (result)`: the bodies of today's
     `eval_lambda_call`/`eval_builtin_call` minus their eval-args loops,
     dispatching on `KIND_LAMBDA`/`KIND_BUILTIN`, raising
     `_UNCALLABLE_EXCEPTION` otherwise (moves the kind switch from
     `vm_eval_list`). Arity checks stay inside, per kind.
  3. Declare `vm_call` in `include/vm.h` under the `vm_eval.c` section, with
     the stack-effect comment.
  4. Rewrite `rkl_map`'s loop body as: `vm_push(vm, LIST_AT(l, i));`
     `vm_pack_list(vm, 1); vm_push(vm, func); vm_call(vm);` — same for
     `rkl_filter`. No packed `(func item)` expression, no `vm_eval_node`.
  5. Add `tests/positive/map_compound.rkl` mapping identity over
     `(list (list 1 2) "s" (list))` plus a `filter` over compound values;
     generate the `.out` after eyeballing the output.
- **Verify**: `make build && make test`, `make debug && make test`;
  `(print (map (lambda (x) x) (list (list 1 2))))` prints `((1 2))` instead of
  erroring; `grep -n "vm_eval_node" src/sparkle_impl/builtins_list.c` returns
  nothing.
- **Risk**: touches the eval hot path — the per-instruction stack-discipline
  asserts in `vm_run` and the full suite under sanitizers are the safety net.
  `rkl_eval` (`builtins_arithmetic_logic.c:149`) intentionally re-enters
  `vm_eval_node` — leave it alone.

### Major

#### R2: Stop `read_file` crashing on a missing input file

- **Severity**: Major
- **Principle**: error-convention misuse (assert on user input)
- **Location**: `src/main.c` — `read_file` (19–35), `assert(file)` at line 21
- **Problem**: `assert` guards `fopen`'s result, but release builds compile with
  `-DNDEBUG`, so `./build/sparkle nosuchfile.rkl` segfaults (reproduced,
  exit 139). Asserts are for internal invariants; a wrong path is routine user
  input on the primary entry path.
- **Change**:
  1. In `read_file`, on `!file`: `fprintf(stderr, ...)` with the path and
     `strerror(errno)`, then `exit(1)` (or return `NULL` and let `main` bail
     before allocating anything). Match the `RED`/`RESET` styling used in
     `diagnostics.c` if printing from `main.c`.
  2. While there, check the `fread` return value against `size` the same way.
- **Verify**: `make build && ./build/sparkle /nonexistent.rkl` prints an error
  and exits 1 (no segfault); `make test` unchanged.
- **Risk**: none; exit code for this failure changes from 139 to 1.

#### R3: Invert the VM → special_forms dependency

- **Severity**: Major
- **Principle**: DIP / layering (stated layering: `sparkle_impl` sits on top of
  the VM)
- **Location**: `src/vm/vm_eval.c` — `#include "special_forms.h"` (line 4),
  `try_dispatch_special_form` call in `vm_eval_list` (line 199);
  `src/sparkle_impl/special_forms.c` — `special_forms_init` (8–11)
- **Problem**: the VM core names and calls a `sparkle_impl` module, so the
  stable evaluator depends on the volatile language layer — backwards relative
  to builtins, which are injected via `vm_register_builtin`. It also drags a
  hidden temporal coupling: `special_forms_init` must mutate the global
  `SPECIAL_FORMS` table (re-pointing `keyword` from string literals to interned
  names) before any eval, or dispatch silently misses and forms fall through to
  `UNDEFINED_EXCEPTION`.
- **Change**:
  1. Add a hook field to `struct VM` in `include/vm.h` (next to
     `StringInterner *si`): `bool (*try_special)(VM *vm, StringName name);`
     initialize it to `NULL` in `vm_alloc` (`src/vm/vm.c:12–39`).
  2. In `vm_eval_list` (vm_eval.c:199) replace the direct call with
     `vm->try_special && vm->try_special(vm, SYMBOL(head))`; delete the
     `special_forms.h` include (line 4).
  3. In `special_forms.c`/`.h`, replace `special_forms_init(StringInterner *)`
     with `special_forms_attach(VM *vm)`: intern the keywords via `vm->si` and
     set `vm->try_special = try_dispatch_special_form`. Drop the now-unused
     `#include <stdio.h>` (special_forms.c:6) in passing.
  4. In `src/main.c:62` call `special_forms_attach(vm)` (it already runs after
     `vm_alloc`).
- **Verify**: `make build && make test`;
  `grep -rn "special_forms" src/vm/` returns nothing.
- **Risk**: none user-visible. A build that forgets to attach loses special
  forms — the same failure mode as forgetting `special_forms_init` today, but
  now reachable only from one call site in `main`.

#### R4: Move `vm_cast_to_string` out of the VM core

- **Severity**: Major
- **Principle**: DIP / layering (VM core → `io.h`), feature envy
- **Location**: `src/vm/vm_eval.c` — `vm_cast_to_string` (127–139),
  `#include "io.h"` (line 2); `include/vm.h` — declaration (line ~127);
  `src/sparkle_impl/builtins_string.c` — `rkl_str` (10–12)
- **Problem**: `vm_cast_to_string` exists in the VM core only by analogy with
  `vm_cast_to_bool`, but the analogy doesn't hold: truthiness is a core
  semantic used by special forms (`if`, `while`, `and`, `or`), while
  string-casting has exactly one caller — the `str` builtin — and forces
  `vm_eval.c` to include `io.h`, an impl-layer header. The VM should not know
  how objects are rendered.
- **Change**:
  1. Move the body of `vm_cast_to_string` into `builtins_string.c` as
     `static void cast_to_string(VM *vm)`; `rkl_str` calls it. Add
     `#include "io.h"` there.
  2. Delete the function from `vm_eval.c`, its `io.h` include, and the
     declaration in `vm.h`.
- **Verify**: `make build && make test` (`cast_to_str.rkl` covers behavior);
  `grep -rn '"io.h"' src/vm/` returns nothing.
- **Risk**: none — no other caller exists
  (`grep -rn vm_cast_to_string src/` before deleting to confirm).
  Do after R3 so the two `vm_eval.c` edits don't collide.

### Minor

#### R5: Give builtins a `vm_peek_at` instead of poking `vm->value_stack`

- **Severity**: Minor
- **Principle**: leaky abstraction / ISP
- **Location**: `src/sparkle_impl/builtins_list.c:38` (`rkl_put`), `:82–83`
  (`rkl_append`); `src/sparkle_impl/builtins_string.c:37` (`rkl_str_sub`)
- **Problem**: three-argument builtins reach past the `vm_stack.c` API straight
  into the DA (`da_at_end(vm->value_stack, 2)`) because the API stops at
  `vm_prev`. Impl-layer code now depends on the stack's concrete layout.
- **Change**:
  1. In `src/vm/vm_stack.c` add `Object *vm_peek_at(VM *vm, size_t n)`
     (`ASSERT_HAS(vm, n + 1); return STACK_AT(vm, n);`), declared in `vm.h`
     next to `vm_prev`, with the `x, y -> x, y` comment style.
  2. Replace the three `da_at_end(vm->value_stack, …)` uses with
     `vm_peek_at(vm, 2)` / `vm_peek_at(vm, 1)`.
- **Verify**: `make build && make test`;
  `grep -rn "value_stack" src/sparkle_impl/` returns nothing.
- **Risk**: none.

#### R6: `vm_register_builtin` claims `StringName` but receives a raw string

- **Severity**: Minor
- **Principle**: contract clarity (type lie)
- **Location**: `src/vm/vm.c:79` — `vm_register_builtin(VM *, StringName, BuiltinObject)`
- **Problem**: callers pass raw literals from `BuiltinDef.name` and the function
  interns internally (`si_get(vm->si, name)`), so the `StringName` parameter
  type is false — `StringName` means "interned, comparable by pointer"
  everywhere else. The in-code `TODO: maybe move si lookup somewhere`
  acknowledges it.
- **Change**:
  1. Change the parameter to `const char *name` in `vm.c:79` and `vm.h`;
     drop the TODO comment. Callers (`REGISTER_MODULE` in
     `src/sparkle_core/builtins.c`) need no change.
- **Verify**: `make build && make test`.
- **Risk**: none.

#### R7: Delete the empty `object.c`

- **Severity**: Minor
- **Principle**: dead code
- **Location**: `src/sparkle_core/object.c` — sole content is `#include "object.h"`
- **Problem**: an empty translation unit; `object.h` is macros and typedefs
  only. It misleads readers into expecting object logic there (the review
  itself went looking).
- **Change**:
  1. `rm src/sparkle_core/object.c` — the Makefile globs sources, nothing else
     to update.
- **Verify**: `make build && make test`.
- **Risk**: none. Alternative (rejected): move `write_expr` there — per
  `CONVENTIONS`-era decisions, textual representation deliberately lives in
  `io.c`.

## Non-findings

- `vm.h` as the single facade header for the seven `vm_*.c` files — cohesive by
  concern, deliberate; splitting it would multiply includes for no gain.
- Open (non-opaque) structs `VM`, `Parser`, `Lexer`, `GC`, `Scope` — the
  project's consistent style; opaque pointers would fight it in a 3.3k-line
  codebase guarded by asserts and sanitizers.
- Exhaustive `switch` on `ObjectKind` in `gc.c` (free, mark), `io.c`
  (`write_expr`), `vm_eval.c` (eval) — one switch per operation, no `default`,
  `-Wswitch-enum` enforced. Correct side of the expression problem here:
  operations grow, the kind set is closed.
- `&&`/`||` builtins coexisting with `and`/`or` special forms — deliberate
  strict vs short-circuit pair, both documented.
- Linear scans in the interner, scopes, and special-form dispatch —
  performance, not architecture; `TODO.md` already tracks interning/allocation
  work.
- `malloc` + `assert` as the OOM policy — documented convention.
- The `try` handler double-invocation inconsistency — already tracked in
  `TODO.md`, not re-litigated here.
- `diagnostics.c` reading `vm->exception` and the exception singletons —
  shell-level error reporting is allowed to see the machine.
- `gc_alloc_object` staying public alongside the typed constructors — needed by
  the `X_RUNTIME_SINGLETONS` init in `vm.c`; harmless.

## Suggested execution order

1. **R1** — the user-visible semantics bug; introduces `vm_call`, which nothing
   else depends on but future work (`apply`, FFI) will.
2. **R3** — layering inversion in `vm_eval.c`; do before R4 to serialize edits
   to the same file.
3. **R4** — completes the "nothing in `src/vm/` includes impl headers"
   invariant; verify with `grep -rn '"io.h"\|special_forms' src/vm/` → empty.
4. **R2**, **R5**, **R6**, **R7** — independent of everything above and of each
   other; safely parallelizable, any order.
