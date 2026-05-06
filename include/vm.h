#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "lisp_node.h"
#include "string_view.h"
#include <setjmp.h>

#define VM_DONE(e_) ((e_)->stmts_count == 0)
#define VM_VALID(e_) (!VM_DONE(e_) && !(e_)->is_err)

#define VM_CURR_SCOPE(e_) (da_at((e_)->scope_stack, (e_)->scope_stack.size - 1))

#define ASSERT_HAS(e_, n_) (assert((e_)->value_stack.size >= (n_)))
#define ASSERT_KIND(e_, k_) (assert(da_at_end((e_)->value_stack, 0)->kind == (k_)))
#define ASSERT_LIST(e_) (assert(da_at_end((e_)->value_stack, 0)->kind == LISP_NIL || \
                                da_at_end((e_)->value_stack, 0)->kind == LISP_CONS))

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

    bool is_err;
};

VM *vm_alloc(LispNodePtrDA exprs, GC *gc);
void vm_free(VM *vm);

void vm_register_builtin(VM *vm, StringView name, LispBuiltin func_ptr);

void vm_eval_expr(VM *vm);
void vm_eval_all(VM *vm);

void vm_recover(VM *vm);
void vm_push_recovery(VM *vm, jmp_buf *jmp);
void vm_pop_recovery(VM *vm);

void vm_push_scope(VM *vm, Scope *scope);
void vm_build_scope(VM *vm);
void vm_pop_scope(VM *vm);
void vm_scope_define(VM *vm, StringView name);
void vm_scope_get(VM *vm, StringView name);

void vm_build_value(VM *vm, LispNodeKind kind);
void vm_build_integer(VM *vm, int value);
void vm_build_builtin(VM *vm, LispBuiltin value);
void vm_build_nil(VM *vm);
void vm_build_lambda(VM *vm, StringViewDA args, bool is_variadic, LispNode *expr, Scope *scope);
void vm_build_symbol(VM *vm, StringView value);
void vm_build_string(VM *vm, StringView value);

void vm_push(VM *vm, LispNode *value);
void vm_swap(VM *vm);
void vm_pop(VM *vm);
void vm_rot(VM *vm);
void vm_pop_prev(VM *vm);
LispNode *vm_peek(VM *vm);

size_t eval_list(VM *vm);
void unpack_cons(VM *vm);
size_t unpack_list(VM *vm);
size_t eval_list_inplace(VM *vm);
void unpack_list_n(VM *vm, size_t n);

void vm_mark(VM *vm);

#endif
