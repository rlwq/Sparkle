#include "vm.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "object.h"

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

VM *vm_alloc(GC *gc, StringInterner *si) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->gc = gc;
    vm->si = si;

    da_init(vm->instructions);
    vm->instruction_ptr = 0;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);
    da_init(vm->recovery_stack);
    da_init(vm->special_forms);

    vm->is_err = false;
    vm->exception = NULL;

#define X(name_, kind_, init_)                                                                     \
    vm->singletons._##name_ = gc_alloc_object(vm->gc, kind_);                                      \
    OBJ_AS(vm->singletons._##name_) = (ObjectUnion)init_;
    X_RUNTIME_SINGLETONS
#undef X

#define X(name_, msg_) vm->singletons._##name_ = gc_alloc_symbol(vm->gc, si_get(vm->si, #name_));
    X_RUNTIME_EXCEPTIONS
#undef X

#define X(name_) vm->symbols._##name_ = gc_alloc_symbol(vm->gc, si_get(vm->si, #name_));
    X_LANGUAGE_SYMBOLS
#undef X

    return vm;
}

void vm_load_instructions(VM *vm, ObjectPtrDA instructions) {
    vm->instructions.size = 0;
    for (size_t i = 0; i < instructions.size; i++)
        da_push(vm->instructions, da_at(instructions, i));

    vm->instruction_ptr = 0;
}

void vm_free(VM *vm) {
    da_free(vm->instructions);
    da_free(vm->scope_stack);
    da_free(vm->value_stack);
    da_free(vm->recovery_stack);
    da_free(vm->special_forms);

    free(vm);
}
void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = vm->instruction_ptr; i < vm->instructions.size; i++)
        gc_mark_object(da_at(vm->instructions, i));

    // Marking scopes
    for (size_t i = 0; i < vm->scope_stack.size; i++)
        gc_mark_scope(da_at(vm->scope_stack, i));

    // Marking value stack
    for (size_t i = 0; i < vm->value_stack.size; i++)
        gc_mark_object(da_at(vm->value_stack, i));

    if (vm->exception)
        gc_mark_object(vm->exception);

// Marking singletons
#define X(name_, kind_, init_) gc_mark_object(vm->singletons._##name_);
    X_RUNTIME_SINGLETONS
#undef X

// Marking exception symbols
#define X(name_, msg_) gc_mark_object(vm->singletons._##name_);
    X_RUNTIME_EXCEPTIONS
#undef X

// Marking language symbols
#define X(name_) gc_mark_object(vm->symbols._##name_);
    X_LANGUAGE_SYMBOLS
#undef X
}

void vm_register_builtin(VM *vm, const char *name, BuiltinObject func_ptr) {
    vm_build_builtin(vm, func_ptr);
    vm_scope_define(vm, si_get(vm->si, name));
    vm_pop(vm);
}

void vm_register_special_form(VM *vm, const char *keyword, void (*func)(VM *vm)) {
    SpecialFormEntry entry = {.keyword = si_get(vm->si, keyword), .func = func};
    da_push(vm->special_forms, entry);
}

bool vm_try_special_form(VM *vm, StringName name) {
    for (size_t i = 0; i < vm->special_forms.size; i++) {
        if (name == da_at(vm->special_forms, i).keyword) {
            da_at(vm->special_forms, i).func(vm);
            return true;
        }
    }

    return false;
}

void vm_run(VM *vm) {
    // On entry, not on exit: callers read both to report the failure.
    vm->is_err = false;
    vm->exception = NULL;

    jmp_buf error_handler;
    vm_push_recovery(vm, &error_handler);

    if (setjmp(error_handler)) {
        vm->is_err = true;
        goto end;
    }

    while (vm->instruction_ptr < vm->instructions.size) {
        assert(vm->instruction_ptr < vm->instructions.size);
        vm_push(vm, da_at(vm->instructions, vm->instruction_ptr));
        vm_eval_object(vm);
        vm_pop(vm);

        assert(vm->recovery_stack.size == 1);
        assert(vm->scope_stack.size == 1);
        assert(vm->value_stack.size == 0);

        vm->instruction_ptr++;
    }
end:
    vm_pop_recovery(vm);
}
