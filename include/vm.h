#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "object.h"
#include "string_interner.h"
#include <setjmp.h>

#define VM_CURR_SCOPE(e_) (da_at_end((e_)->scope_stack, 0))

#define ASSERT_KIND(e_, k_)                                                                        \
    (assert(((e_)->value_stack.size > 0) && (da_at_end((e_)->value_stack, 0)->kind == (k_))))

#define VM_RECOVER_IF(vm_, expr_, ex_)                                                             \
    if ((expr_))                                                                                   \
        vm_recover(vm_, ex_);

#define SINGLETONS                                                                                 \
    X(Nil, KIND_NIL, {0})                                                                          \
    X(True, KIND_BOOL, {.bool_ = true})                                                            \
    X(False, KIND_BOOL, {.bool_ = false})

typedef struct {
    jmp_buf *jmp;
    size_t values_count;
    size_t scopes_count;
} RecoveryStackEntry;

struct VM {
    ObjectPtrDA instructions;
    size_t instruction_ptr;

    DA(Scope *) scope_stack;
    DA(Object *) value_stack;
    DA(RecoveryStackEntry) recovery_stack;

    GC *gc;
    StringInterner *si;

    struct {
#define X(name_, kind_, init_) Object *_##name_;
        SINGLETONS
#undef X
    } singletons;

    ExceptionKind exception;
    bool is_err;
};

// vm.c
VM *vm_alloc(ObjectPtrDA instructions, GC *gc, StringInterner *si);
void vm_free(VM *vm);
void vm_mark(VM *vm);
void vm_register_builtin(VM *vm, StringName name, BuiltinObject func_ptr);
void vm_run(VM *vm);

// vm_recovery.c
void vm_push_recovery(VM *vm, jmp_buf *jmp);
void vm_pop_recovery(VM *vm);
void vm_recover(VM *vm, ExceptionKind exception) __attribute__((cold));
void vm_expect(VM *vm, ObjectType type) __attribute__((cold));
void vm_expect2(VM *vm, ObjectType prev, ObjectType peek) __attribute__((cold));

// vm_scope.c
void vm_build_scope(VM *vm);
void vm_push_scope(VM *vm, Scope *scope);
void vm_pop_scope(VM *vm);
bool vm_scope_define(VM *vm, StringName name);
void vm_scope_get(VM *vm, StringName name);
void vm_scope_set(VM *vm, StringName name);

// vm_build.c
void vm_build_value(VM *vm, ObjectKind kind);
void vm_build_integer(VM *vm, Integer value);
void vm_build_bool(VM *vm, bool value);
void vm_build_float(VM *vm, double value);
void vm_build_exception(VM *vm, ExceptionKind value);
void vm_build_builtin(VM *vm, BuiltinObject value);
void vm_build_nil(VM *vm);
void vm_build_lambda(VM *vm, LambdaArgs args, bool is_variadic, Object *expr, Scope *scope);
void vm_build_symbol(VM *vm, StringName value);
void vm_build_string(VM *vm, StringName value);
void vm_build_cons(VM *vm, Object *car, Object *cdr);

// vm_stack.c
void vm_push(VM *vm, Object *value);
void vm_push_prev(VM *vm, Object *value);
Object *vm_peek(VM *vm);
Object *vm_prev(VM *vm);
void vm_dup(VM *vm);
void vm_dup_prev(VM *vm);
void vm_swap(VM *vm);
void vm_rot(VM *vm);
void vm_pop(VM *vm);
void vm_pop_n(VM *vm, size_t n);
void vm_pop_prev(VM *vm);
void vm_pop_prev_n(VM *vm, size_t n);

// vm_ops.c
void vm_pack_cons(VM *vm);
void vm_unpack_cons(VM *vm);
size_t vm_unpack_list(VM *vm);
void vm_unpack_list_n(VM *vm, size_t n);

// vm_eval.c
void vm_eval_node(VM *vm);
void vm_eval_cons(VM *vm);
void vm_eval_symbol(VM *vm);
size_t vm_eval_list(VM *vm);
bool vm_cast_to_bool(VM *vm);
ObjectKind vm_to_common_numeric(VM *vm);

#endif
