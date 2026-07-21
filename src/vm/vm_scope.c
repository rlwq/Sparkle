#include "gc.h"
#include "scope.h"
#include "vm.h"

void vm_build_scope(VM *vm) {
    // Collect before allocating so scope creation participates in the capacity
    // check like object creation. The parent (VM_CURR_SCOPE) is already on the
    // scope stack and thus stays rooted across the collection.
    vm_maybe_collect(vm);
    vm_push_scope(vm, gc_alloc_scope(vm->gc, VM_CURR_SCOPE(vm)));
}

void vm_push_scope(VM *vm, Scope *scope) {
    da_push(vm->scope_stack, scope);
}

void vm_pop_scope(VM *vm) {
    assert(vm->scope_stack.size > 0);
    da_pop(vm->scope_stack);
}

void vm_scope_define(VM *vm, StringName name) {
    bool result = scope_define(VM_CURR_SCOPE(vm), name, vm_peek(vm));
    if (!result)
        vm_recover(vm, vm->singletons._REBINDING_EXCEPTION);
}

void vm_scope_get(VM *vm, StringName name) {
    Object *lookup_result = scope_get(VM_CURR_SCOPE(vm), name);
    if (!lookup_result) {
        vm_recover(vm, vm->singletons._UNDEFINED_EXCEPTION);
    }
    vm_push(vm, lookup_result);
}

// Object -> Object
void vm_scope_set(VM *vm, StringName name) {
    bool result = scope_set(VM_CURR_SCOPE(vm), name, vm_peek(vm));
    if (!result)
        vm_recover(vm, vm->singletons._UNDEFINED_EXCEPTION);
}
