#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "object.h"
#include "vm.h"

VM *vm_alloc(ObjectPtrDA instructions, GC *gc, StringInterner *si) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->instructions = instructions;
    vm->instruction_ptr = 0;

    vm->gc = gc;
    vm->si = si;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);
    da_init(vm->recovery_stack);

    vm->is_err = false;

#define X(name_, kind_, init_)                                                                     \
    vm->singletons._##name_ = gc_alloc_node(vm->gc, kind_);                                        \
    vm->singletons._##name_->as = (ObjectUnion)init_;
    SINGLETONS
#undef X

    return vm;
}

void vm_free(VM *vm) {
    da_free(vm->scope_stack);
    da_free(vm->value_stack);
    da_free(vm->recovery_stack);

    free(vm);
}
void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = vm->instruction_ptr; i < vm->instructions.size; i++)
        gc_mark_node(da_at(vm->instructions, i));

    // Marking scopes
    for (size_t i = 0; i < vm->scope_stack.size; i++)
        gc_mark_scope(da_at(vm->scope_stack, i));

    // Marking value stack
    for (size_t i = 0; i < vm->value_stack.size; i++)
        gc_mark_node(da_at(vm->value_stack, i));

// Marking singletons
#define X(name_, kind_, init_) gc_mark_node(vm->singletons._##name_);
    SINGLETONS
#undef X
}

void vm_register_builtin(VM *vm, StringName name, BuiltinObject func_ptr) {
    vm_build_builtin(vm, func_ptr);
    // TODO: maybe move si lookup somewhere
    vm_scope_define(vm, si_get(vm->si, name));
    vm_pop(vm);
}

void vm_run(VM *vm) {
    jmp_buf error_handler;
    vm_push_recovery(vm, &error_handler);

    if (setjmp(error_handler)) {
        vm->is_err = true;
        goto end;
    }

    while (vm->instruction_ptr < vm->instructions.size) {
        assert(vm->instruction_ptr < vm->instructions.size);
        vm_push(vm, da_at(vm->instructions, vm->instruction_ptr));
        vm_eval_node(vm);
        vm_pop(vm);

        assert(vm->recovery_stack.size == 1);
        assert(vm->scope_stack.size == 1);
        assert(vm->value_stack.size == 0);

        vm->instruction_ptr++;
    }
end:
    vm_pop_recovery(vm);
}
