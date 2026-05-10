#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lisp_node.h"
#include "scope.h"
#include "vm.h"

VM *vm_alloc(LispNodePtrDA exprs, GC *gc, StringInterner *si) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->stmts = exprs.data;
    vm->stmts_count = exprs.size;

    vm->gc = gc;
    vm->si = si;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);
    da_init(vm->recovery_stack);

    vm->is_err = false;

#define X(name_, kind_, init_)                                                                     \
    vm->singletons._##name_ = gc_alloc_node(vm->gc, kind_);                                        \
    vm->singletons._##name_->as = (LispNodeUnion)init_;
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

void vm_push_recovery(VM *vm, jmp_buf *jmp) {
    RecoveryStackEntry recovery = (RecoveryStackEntry){
        .jmp = jmp, .scopes_count = vm->scope_stack.size, .values_count = vm->value_stack.size};
    da_push(vm->recovery_stack, recovery);
}

void vm_pop_recovery(VM *vm) {
    assert(vm->recovery_stack.size > 0);
    da_pop(vm->recovery_stack);
}

void vm_push_scope(VM *vm, Scope *scope) {
    da_push(vm->scope_stack, scope);
}

void vm_build_scope(VM *vm) {
    vm_push_scope(vm, gc_alloc_scope(vm->gc, VM_CURR_SCOPE(vm)));
}

void vm_pop_scope(VM *vm) {
    assert(vm->scope_stack.size > 0);
    da_pop(vm->scope_stack);
}

void vm_scope_define(VM *vm, StringName name) {
    scope_define(VM_CURR_SCOPE(vm), name, vm_peek(vm));
}

void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = 0; i < vm->stmts_count; i++)
        gc_mark_node(vm->stmts[i]);

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

LispNode *vm_advance(VM *vm) {
    assert(vm->stmts_count > 0);
    LispNode *curr = *(vm->stmts);

    vm->stmts++;
    vm->stmts_count--;
    return curr;
}

void vm_recover(VM *vm, ExceptionKind value) {
    assert(vm->recovery_stack.size > 0);

    RecoveryStackEntry recovery = da_at_end(vm->recovery_stack, 0);

    vm->value_stack.size = recovery.values_count;
    vm->scope_stack.size = recovery.scopes_count;
    vm->exception = value;

    longjmp(*(recovery.jmp), 1);
}

void vm_expect(VM *vm, LispNodeType type) {
    ASSERT_HAS(vm, 1);
    VM_RECOVER_IF(vm, !((1 << vm_peek(vm)->kind) & type), WRONG_TYPE);
}

void vm_expect2(VM *vm, LispNodeType prev, LispNodeType peek) {
    ASSERT_HAS(vm, 1);
    VM_RECOVER_IF(vm, !((1 << vm_peek(vm)->kind) & peek), WRONG_TYPE);
    VM_RECOVER_IF(vm, !((1 << vm_prev(vm)->kind) & prev), WRONG_TYPE);
}

void vm_register_builtin(VM *vm, StringName name, LispBuiltin func_ptr) {
    vm_build_builtin(vm, func_ptr);
    // TODO: maybe move si lookup somewhere
    vm_scope_define(vm, si_get(vm->si, name));
    vm_pop(vm);
}

void vm_eval_all(VM *vm) {
    assert(VM_VALID(vm));

    jmp_buf error_handler;
    vm_push_recovery(vm, &error_handler);

    if (setjmp(error_handler)) {
        vm->is_err = true;
    } else {
        while (VM_VALID(vm)) {
            vm_push(vm, vm_advance(vm));
            vm_eval_node(vm);
            vm_pop(vm);
            assert(vm->recovery_stack.size == 1);
            assert(vm->scope_stack.size == 1);
            assert(vm->value_stack.size == 0);
        }
    }
    vm_pop_recovery(vm);
}

// Node (Cons) -> Node (Tail), Node (Head)
void vm_unpack_cons(VM *vm) {
    ASSERT_HAS(vm, 1);
    ASSERT_KIND(vm, LISP_CONS);

    vm_push_prev(vm, CDR(vm_peek(vm)));
    vm_push_prev(vm, CAR(vm_peek(vm)));
    vm_pop(vm);
}

// Node (Tail), Node (Head) -> Node (Cons)
void vm_pack_cons(VM *vm) {
    ASSERT_HAS(vm, 2);
    vm_build_cons(vm, vm_peek(vm), vm_prev(vm));
    vm_pop_prev_n(vm, 2);
}

void vm_scope_get(VM *vm, StringName name) {
    LispNode *lookup_result = scope_get(VM_CURR_SCOPE(vm), name);
    if (!lookup_result) {
        vm_recover(vm, SYMBOL_UNDEFINED);
        return;
    }
    vm_push(vm, lookup_result);
}

// Node -> Node
void vm_scope_set(VM *vm, StringName name) {
    bool result = scope_set(VM_CURR_SCOPE(vm), name, vm_peek(vm));
    if (!result)
        vm_recover(vm, SYMBOL_UNDEFINED);
}
