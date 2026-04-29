#include <assert.h>
#include <stdio.h>
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

    da_init(vm->results);
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
LispNode *vm_pop_value(VM *vm) {
    assert(vm->value_stack.size);

    LispNode *curr = da_at(vm->value_stack, vm->value_stack.size-1);
    da_pop(vm->value_stack);
    return curr;
}

LispNode *vm_peek_value(VM *vm) {
    assert(vm->value_stack.size);

    return da_at(vm->value_stack, vm->value_stack.size-1);
}

void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = 0; i < vm->stmts_count; i++)
        gc_mark_node(vm->stmts[i]);

    // Marking results
    for (size_t i = 0; i < vm->results.size; i++)
        gc_mark_node(da_at(vm->results, i));

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

void eval_expr(LispNode *expr, VM *vm);

size_t eval_list(LispNode *expr, VM *vm) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);
    
    size_t size = 0;
    for (LispNode *curr = expr; curr->kind != LISP_NIL; curr = CDR(curr)) {
        assert(curr->kind == LISP_CONS);
        eval_expr(CAR(curr), vm);
        size++;
    }
    return size;
}

void eval_current(VM *vm) {
    assert(VM_VALID(vm));
    
    LispNode *stmt = vm_advance(vm);
    eval_expr(stmt, vm);
    LispNode *node = vm_pop_value(vm);

    if (node) da_push(vm->results, node);
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

void eval_let_form(LispNode *symbol, LispNode *expr, VM *vm) {
    assert(symbol->kind == LISP_SYMBOL);
    eval_expr(expr, vm);

    scope_define(CURR_SCOPE(vm), symbol->as.symbol, vm_peek_value(vm));
}

void eval_if_form(LispNode *condition, LispNode *if_true, LispNode *if_false, VM *vm) {
    eval_expr(condition, vm);
    if (vm_pop_value(vm)->kind == LISP_NIL) {
        eval_expr(if_false, vm); 
        return;
    }
    eval_expr(if_true, vm);
}

void eval_lambda_form(StringViewDA args, LispNode *subexpr, VM *vm) {
    LispNode *lambda_result = gc_alloc_node(vm->gc, LISP_LAMBDA);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.scope = CURR_SCOPE(vm);

    vm_push_value(vm, lambda_result);
}

void eval_lambda_call(LispNode *lambda, VM *vm) {
    vm_push_scope(vm, lambda->as.lambda.scope);
    vm_push_scope(vm, gc_alloc_scope(vm->gc, CURR_SCOPE(vm)));

    for (size_t i = 0; i < lambda->as.lambda.args.size; i++)
        scope_define(CURR_SCOPE(vm), da_at(lambda->as.lambda.args, lambda->as.lambda.args.size - i - 1),
                   vm_pop_value(vm));

    eval_expr(lambda->as.lambda.expr, vm);
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

        eval_if_form(condition, if_true, if_false, vm);
        return true;
    }

    if (sv_eq(head->as.symbol, sv_mk("let"))) {
        assert(args->kind == LISP_CONS);

        LispNode *symbol = args->as.cons.car;
        LispNode *value_cons = args->as.cons.cdr;

        assert(symbol->kind == LISP_SYMBOL);
        assert(value_cons->kind == LISP_CONS);
        assert(value_cons->as.cons.cdr->kind == LISP_NIL);

        eval_let_form(symbol, value_cons->as.cons.car, vm);
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
        eval_lambda_form(lambda_args, lambda_subexpr, vm);
        return true;
    }
    
    return false;
}

void eval_cons(LispNode *expr, VM *vm) {
    assert(expr->kind == LISP_CONS);

    LispNode *head = expr->as.cons.car; 
    LispNode *args = expr->as.cons.cdr; 

    bool is_special_form = dispatch_special_form(head, args, vm);

    if (is_special_form) return;

    eval_expr(head, vm);
    LispNode *evaluated_head = vm_pop_value(vm);

    size_t args_count = eval_list(args, vm);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            eval_lambda_call(evaluated_head, vm);
            return;
        break;

        case LISP_BUILTIN:
            evaluated_head->as.builtin(args_count, vm);
            return;
        break;

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
            assert(0 && "Can't call this kind of object.");
        break;
    }

    UNREACHABLE();
}

void eval_expr(LispNode *expr, VM *vm) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            vm_push_value(vm, expr);
            return;
        break;

        case LISP_SYMBOL:
            if (sv_eq(expr->as.symbol, sv_mk("NIL")))
                vm_push_value(vm, gc_alloc_node(vm->gc, LISP_NIL));
            else vm_push_value(vm, scope_get(CURR_SCOPE(vm), expr->as.symbol));
            return;
        break;

        case LISP_CONS:
            eval_cons(expr, vm);
            return;
        break;
    }
    
    UNREACHABLE();
}

LispNodePtrDA extract_results(VM *vm) {
    LispNodePtrDA results = vm->results;
    da_nullify(vm->results);
    return results;
}

