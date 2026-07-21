#include "gc.h"
#include "vm.h"

// The single VM-driven collection point. Raw gc_alloc_* never collect on their
// own (see gc.h); every VM-level allocation path must funnel through here so the
// capacity check is applied consistently to both objects and scopes, and only at a
// point where the value/scope stacks are fully rooted.
void vm_maybe_collect(VM *vm) {
    if (gc_grow_if_needed(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
}

// Every allocating vm_build_ constructor is collect-then-alloc-then-push: the
// sweep runs while everything live is rooted and the new object does not exist
// yet, so a fresh object can never be collected before it is reachable.
// vm_build_bool and vm_build_nil are the exceptions and allocate nothing - they
// push a singleton that vm_alloc made once, so there is nothing to collect for.
void vm_build_integer(VM *vm, Integer value) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_integer(vm->gc, value));
}

void vm_build_symbol(VM *vm, StringName value) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_symbol(vm->gc, value));
}

// data may point into another string's buffer as long as that string is rooted:
// the collection happens before the copy, while the source is still alive.
void vm_build_string(VM *vm, const char *data, size_t size) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_string(vm->gc, data, size));
}

// Adopts data (see gc_alloc_string_own). The buffer is plain malloc'd memory,
// not yet owned by the GC, so it safely survives the collection.
void vm_build_string_own(VM *vm, char *data, size_t size) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_string_own(vm->gc, data, size));
}

void vm_build_list(VM *vm) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_list(vm->gc));
}

void vm_build_bool(VM *vm, bool value) {
    if (value)
        vm_push(vm, vm->singletons._True);
    else
        vm_push(vm, vm->singletons._False);
}

void vm_build_float(VM *vm, double value) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_float(vm->gc, value));
}

void vm_build_builtin(VM *vm, BuiltinObject value) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_builtin(vm->gc, value));
}

void vm_build_nil(VM *vm) {
    vm_push(vm, vm->singletons._Nil);
}

void vm_build_lambda(VM *vm, bool is_variadic, Object *expr, Scope *scope) {
    vm_maybe_collect(vm);
    vm_push(vm, gc_alloc_lambda(vm->gc, is_variadic, expr, scope));
}
