#ifndef VM_H
#define VM_H

#include "lisp_node.h"
#include "string_view.h"
#include "forwards.h"

#define VM_DONE(e_) ((e_)->stmts_count == 0)
#define VM_VALID(e_) (!VM_DONE(e_) && !(e_)->is_err)

#define CURR_SCOPE(e_) (da_at((e_)->scope_stack, (e_)->scope_stack.size-1))

struct VM {
    LispNode **stmts;
    size_t stmts_count;

    DA(Scope *) scope_stack;
    DA(LispNode *) value_stack;

    GC *gc;

    bool is_err;
};

VM *vm_alloc(LispNodePtrDA exprs, GC *gc);
void vm_free(VM *vm);

void vm_register_builtin(VM *vm, StringView name, LispBuiltin func_ptr);

void eval_current(VM *vm);
void eval_all(VM *vm);

void vm_push_scope(VM *vm, Scope *scope);
void vm_pop_scope(VM *vm);

void vm_push_value(VM *vm, LispNode *value);
void vm_swap_value(VM *vm);
void vm_pop_value(VM *vm);
LispNode *vm_peek_value(VM *vm);

void vm_mark(VM *vm);

#endif
