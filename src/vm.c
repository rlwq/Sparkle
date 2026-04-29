#include <assert.h>
#include <stdlib.h>

#include "gc.h"
#include "scope.h"
#include "vm.h"
#include "dynamic_array.h"
#include "lisp_node.h"
#include "string_view.h"
#include "forwards.h"
#include "utils.h"

#define CURR(e_) (*((e_)->stmts))

VM *vm_alloc(LispNodePtrDA exprs, GC *gc) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->stmts = exprs.data;
    vm->stmts_count = exprs.size;
    vm->is_err = false;

    vm->gc = gc;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);

    return vm;
}

void vm_free(VM *vm) {
    da_free(vm->scope_stack);
    da_free(vm->value_stack);

    free(vm);
}

void vm_push_scope(VM *vm, Scope *scope) {
    da_push(vm->scope_stack, scope);
}

void vm_pop_scope(VM *vm) {
    assert(vm->scope_stack.size);
    da_pop(vm->scope_stack);
}

// 0 -> 1
void vm_push_value(VM *vm, LispNode *value) {
    da_push(vm->value_stack, value);
}

// 1 -> 0
void vm_pop_value(VM *vm) {
    assert(vm->value_stack.size);

    da_pop(vm->value_stack);
}

// Node, Node -> Node, Node
void vm_swap_value(VM *vm) {
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

LispNode *vm_peek_value(VM *vm) {
    assert(vm->value_stack.size);

    return da_at(vm->value_stack, vm->value_stack.size-1);
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
}

LispNode *vm_advance(VM *vm) {
    assert(VM_VALID(vm));
    LispNode *curr = CURR(vm);
    
    vm->stmts++;
    vm->stmts_count--;
    return curr;
}

void eval_expr(VM *vm);

// Cons -> Expr * n
size_t eval_list(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_NIL ||
           vm_peek_value(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek_value(vm)->kind != LISP_NIL) {
        assert(vm_peek_value(vm)->kind == LISP_CONS);
        // Evaluate head
        vm_push_value(vm, CAR(vm_peek_value(vm)));
        eval_expr(vm);
        vm_swap_value(vm);

        // Push tail
        vm_push_value(vm, CDR(vm_peek_value(vm)));
        vm_swap_value(vm);
        vm_pop_value(vm);
        size++;
    }
    vm_pop_value(vm); 
    return size;
}

void eval_current(VM *vm) {
    assert(VM_VALID(vm));
    
    vm_push_value(vm, vm_advance(vm));
    eval_expr(vm);
    vm_pop_value(vm);
}

void vm_register_builtin(VM *vm, StringView name, LispBuiltin func_ptr) {
    LispNode *builtin = gc_alloc_node(vm->gc, LISP_BUILTIN);
    builtin->as.builtin = func_ptr;

    scope_define(CURR_SCOPE(vm), name, builtin);
}

void eval_all(VM *vm) {
    assert(VM_VALID(vm));

    while (VM_VALID(vm)) {
        eval_current(vm);
        vm_mark(vm);

        gc_sweep(vm->gc);
    }
}

// Node (value), Symbol (name) -> Node
void eval_let_form(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);
    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);
    eval_expr(vm);

    scope_define(CURR_SCOPE(vm), name, vm_peek_value(vm));
}

// Node (is_false), Node (is_true), Node (condition) -> result
void eval_if_form(VM *vm) {
    eval_expr(vm);

    bool is_positive = vm_peek_value(vm)->kind != LISP_NIL;
    vm_pop_value(vm);

    if (is_positive) {
        vm_swap_value(vm);
        vm_pop_value(vm);
    } else
        vm_pop_value(vm);

    eval_expr(vm);
}

// Node -> Lambda
void eval_lambda_form(VM *vm, StringViewDA args) {
    LispNode *subexpr = vm_peek_value(vm);
    LispNode *lambda_result = gc_alloc_node(vm->gc, LISP_LAMBDA);

    vm_push_value(vm, lambda_result);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.scope = CURR_SCOPE(vm);
    
    vm_swap_value(vm);
    vm_pop_value(vm);
}

// Node * n (args), Node (lambda) -> Node (result)
void eval_lambda_call(VM *vm) {
    LispNode *lambda = vm_peek_value(vm);

    vm_push_scope(vm, lambda->as.lambda.scope);
    vm_push_scope(vm, gc_alloc_scope(vm->gc, CURR_SCOPE(vm)));

    for (size_t i = 0; i < lambda->as.lambda.args.size; i++) {
        vm_swap_value(vm);
        scope_define(CURR_SCOPE(vm),
                     da_at(lambda->as.lambda.args,
                           lambda->as.lambda.args.size - i - 1),
                     vm_peek_value(vm));
        vm_pop_value(vm);
    }
    
    vm_push_value(vm, lambda->as.lambda.expr);
    eval_expr(vm);

    vm_swap_value(vm);
    vm_pop_value(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);
}

bool dispatch_special_form(LispNode *head, LispNode *args, VM *vm) {
    if (head->kind != LISP_SYMBOL) return false;

    if (sv_eq(head->as.symbol, sv_mk("if"))) {
        //TODO: make proper assertions
        LispNode *condition = CAR(args);
        LispNode *if_true = CAR(CDR(args));
        LispNode *if_false = CAR(CDR(CDR(args)));
        
        vm_push_value(vm, if_false);
        vm_push_value(vm, if_true);
        vm_push_value(vm, condition);

        eval_if_form(vm);

        vm_swap_value(vm);
        vm_pop_value(vm);

        return true;
    }

    if (sv_eq(head->as.symbol, sv_mk("let"))) {
        assert(args->kind == LISP_CONS);

        LispNode *symbol = CAR(args);
        LispNode *value = CAR(CDR(args));
        
        vm_push_value(vm, value);
        vm_push_value(vm, symbol);
        eval_let_form(vm);

        vm_swap_value(vm);
        vm_pop_value(vm);

        return true;
    }

    if (sv_eq(head->as.symbol, sv_mk("lambda"))) {
        // TODO: make proper assertions
        LispNode *lambda_args_list = CAR(args);
        LispNode *lambda_subexpr = CAR(CDR(args));

        StringViewDA lambda_args;
        da_init(lambda_args);

        for (; lambda_args_list->kind != LISP_NIL;
               lambda_args_list = lambda_args_list->as.cons.cdr)
            da_push(lambda_args, CAR(lambda_args_list)->as.symbol);
        vm_push_value(vm, lambda_subexpr);
        eval_lambda_form(vm, lambda_args);

        vm_swap_value(vm);
        vm_pop_value(vm);

        return true;
    }
    
    return false;
}

// Node (Cons) -> Node
void eval_cons(VM *vm) {
    LispNode *expr = vm_peek_value(vm);
    assert(expr->kind == LISP_CONS);

    LispNode *head = expr->as.cons.car; 
    LispNode *args = expr->as.cons.cdr; 

    bool is_special_form = dispatch_special_form(head, args, vm);

    if (is_special_form) return;

    vm_push_value(vm, head);

    eval_expr(vm);
    LispNode *evaluated_head = vm_peek_value(vm);
    vm_pop_value(vm);

    vm_push_value(vm, args);

    size_t args_count = eval_list(vm);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            vm_push_value(vm, evaluated_head);
            eval_lambda_call(vm);
        break;

        case LISP_BUILTIN:
            evaluated_head->as.builtin(vm, args_count);
        break;

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
            assert(0 && "Can't call this kind of object.");
        break;
    }
    
    vm_swap_value(vm);
    vm_pop_value(vm);
}

// Symbol -> Node
void eval_symbol(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);
    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);

    if (sv_eq(name, sv_mk("NIL"))) vm_push_value(vm, gc_alloc_node(vm->gc, LISP_NIL));
    else vm_push_value(vm, scope_get(CURR_SCOPE(vm), name));
}

// Node -> Node
void eval_expr(VM *vm) {
    switch (vm_peek_value(vm)->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            // Do nothing
            return;
        break;

        case LISP_SYMBOL:
            eval_symbol(vm);
            return;
        break;

        case LISP_CONS:
            eval_cons(vm);
            return;
        break;
    }
    
    UNREACHABLE();
}

