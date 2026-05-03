#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "lisp_node.h"
#include "string_view.h"
#include <setjmp.h>

#define VM_DONE(e_) ((e_)->stmts_count == 0)
#define VM_VALID(e_) (!VM_DONE(e_) && !(e_)->is_err)

#define VM_CURR_SCOPE(e_) (da_at((e_)->scope_stack, (e_)->scope_stack.size - 1))

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

typedef void (*SpecialFormHandler)(VM *vm);

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
void vm_build_lambda(VM *vm, StringViewDA args, LispNode *expr, Scope *scope);
void vm_build_symbol(VM *vm, StringView value);
void vm_build_string(VM *vm, StringView value);

void vm_push_value(VM *vm, LispNode *value);
void vm_swap_value(VM *vm);
void vm_pop_value(VM *vm);
void vm_pop_prev_value(VM *vm);
LispNode *vm_peek_value(VM *vm);

void vm_mark(VM *vm);

#endif
