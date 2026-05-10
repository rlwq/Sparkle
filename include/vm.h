#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "lisp_node.h"
#include "string_interner.h"
#include <setjmp.h>

#define VM_DONE(e_) ((e_)->stmts_count == 0)
#define VM_VALID(e_) (!VM_DONE(e_) && !(e_)->is_err)

#define VM_CURR_SCOPE(e_) (da_at((e_)->scope_stack, (e_)->scope_stack.size - 1))

#define VM_STACK_AT(e_, n_) (da_at_end((e_)->value_stack, (n_)))

#define ASSERT_HAS(e_, n_) (assert((e_)->value_stack.size >= (n_)))
#define ASSERT_KIND(e_, k_)                                                                        \
    (assert(((e_)->value_stack.size > 0) && (da_at_end((e_)->value_stack, 0)->kind == (k_))))
#define ASSERT_LIST(e_)                                                                            \
    (assert(da_at_end((e_)->value_stack, 0)->kind == LISP_NIL ||                                   \
            da_at_end((e_)->value_stack, 0)->kind == LISP_CONS))

#define VM_RECOVER_IF(vm_, expr_, ex_)                                                             \
    if ((expr_))                                                                                   \
        vm_recover(vm_, ex_);

#define SINGLETONS                                                                                 \
    X(Nil, LISP_NIL, {0})                                                                          \
    X(True, LISP_BOOL, {.bool_ = true})                                                            \
    X(False, LISP_BOOL, {.bool_ = false})

typedef struct {
    jmp_buf *jmp;
    size_t values_count;
    size_t scopes_count;
} RecoveryStackEntry;

struct VM {
    LispNode **stmts;
    size_t stmts_count;

    DA(Scope *) scope_stack;
    DA(LispNode *) value_stack;
    DA(RecoveryStackEntry) recovery_stack;

    GC *gc;
    StringInterner *si;

    struct {
#define X(name_, kind_, init_) LispNode *_##name_;
        SINGLETONS
#undef X
    } singletons;

    ExceptionKind exception;
    bool is_err;
};

VM *vm_alloc(LispNodePtrDA exprs, GC *gc, StringInterner *si);
void vm_free(VM *vm);

void vm_register_builtin(VM *vm, StringName name, LispBuiltin func_ptr);
void vm_eval_all(VM *vm);

void vm_push_recovery(VM *vm, jmp_buf *jmp);
void vm_pop_recovery(VM *vm);
void vm_recover(VM *vm, ExceptionKind exception);
void vm_expect_kind(VM *vm, LispNodeKind kind, ExceptionKind exception);

void vm_push_scope(VM *vm, Scope *scope);
void vm_build_scope(VM *vm);
void vm_pop_scope(VM *vm);
void vm_scope_define(VM *vm, StringName name);
void vm_scope_get(VM *vm, StringName name);
void vm_scope_set(VM *vm, StringName name);

void vm_build_value(VM *vm, LispNodeKind kind);
void vm_build_integer(VM *vm, Integer value);
void vm_build_bool(VM *vm, bool value);
void vm_build_float(VM *vm, double value);
void vm_build_exception(VM *vm, ExceptionKind value);
void vm_build_builtin(VM *vm, LispBuiltin value);
void vm_build_nil(VM *vm);
void vm_build_lambda(VM *vm, LambdaArgs args, bool is_variadic, LispNode *expr, Scope *scope);
void vm_build_symbol(VM *vm, StringName value);
void vm_build_string(VM *vm, StringName value);
void vm_build_cons(VM *vm, LispNode *car, LispNode *cdr);

void vm_dup(VM *vm);
void vm_dup_prev(VM *vm);
void vm_push(VM *vm, LispNode *value);
void vm_push_prev(VM *vm, LispNode *value);
void vm_swap(VM *vm);
void vm_pop(VM *vm);
void vm_pop_n(VM *vm, size_t n);
void vm_rot(VM *vm);
void vm_pop_prev(VM *vm);
void vm_pop_prev_n(VM *vm, size_t n);
LispNode *vm_peek(VM *vm);
LispNode *vm_prev(VM *vm);

void vm_eval_node(VM *vm);
void vm_eval_cons(VM *vm);
void vm_eval_symbol(VM *vm);
size_t vm_eval_list(VM *vm);

void vm_pack_cons(VM *vm);
void vm_unpack_cons(VM *vm);
size_t vm_unpack_list(VM *vm);
void vm_unpack_list_n(VM *vm, size_t n);

bool vm_cast_to_bool(VM *vm);
LispNodeKind vm_to_common_numeric(VM *vm);

void vm_mark(VM *vm);

#endif
