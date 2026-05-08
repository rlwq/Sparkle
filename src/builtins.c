#include <stdbool.h>
#include <stdio.h>

#include "builtins.h"
#include "debug.h"
#include "forwards.h"
#include "lisp_node.h"
#include "vm.h"

// Node, Node -> Node
void rkl_eq(VM *vm) {
    ASSERT_HAS(vm, 2);

    bool is_equal = false;

    if (vm_peek(vm)->kind != vm_prev(vm)->kind) {
        is_equal = false;
    }

    switch (vm_peek(vm)->kind) {
    case LISP_NIL:
        is_equal = true;
        break;
    case LISP_SYMBOL:
        is_equal = vm_peek(vm)->as.symbol == vm_prev(vm)->as.symbol;
        break;
    case LISP_INTEGER:
        is_equal = vm_peek(vm)->as.integer == vm_prev(vm)->as.integer;
        break;
    case LISP_EXCEPTION:
        is_equal = vm_peek(vm)->as.exception == vm_prev(vm)->as.exception;
        break;
    case LISP_CONS:
    case LISP_STRING:
    case LISP_BUILTIN:
    case LISP_LAMBDA:
        is_equal = false;
        break;
    }

    vm_pop(vm);
    vm_pop(vm);
    if (is_equal)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

// Integer, Integer -> Integer
void rkl_sub(VM *vm) {
    ASSERT_HAS(vm, 2);

    vm_expect_kind(vm, LISP_INTEGER, WRONG_TYPE);
    Integer value2 = vm_peek(vm)->as.integer;
    vm_pop(vm);

    vm_expect_kind(vm, LISP_INTEGER, WRONG_TYPE);
    Integer value1 = vm_peek(vm)->as.integer;
    vm_pop(vm);

    vm_build_integer(vm, value1 - value2);
}

void rkl_print(VM *vm) {
    LispNode *list = vm_peek(vm);
    while (list->kind == LISP_CONS) {
        print_expr(CAR(list));
        printf("\n");
        list = CDR(list);
    }
    vm_pop(vm);
    vm_build_nil(vm);
}

// Node, Node -> Cons
void rkl_cons(VM *vm) {
    ASSERT_HAS(vm, 2);

    vm_build_value(vm, LISP_CONS);
    vm_peek(vm)->as.cons.cdr = vm_prev(vm);
    vm_pop_prev(vm);
    vm_peek(vm)->as.cons.car = vm_prev(vm);
    vm_pop_prev(vm);
}

// Cons -> Node
void rkl_car(VM *vm) {
    ASSERT_HAS(vm, 1);

    vm_expect_kind(vm, LISP_CONS, WRONG_TYPE);
    vm_push(vm, CAR(vm_peek(vm)));
    vm_pop_prev(vm);
}

// Cons -> Node
void rkl_cdr(VM *vm) {
    ASSERT_HAS(vm, 1);

    vm_expect_kind(vm, LISP_CONS, WRONG_TYPE);
    vm_push(vm, CDR(vm_peek(vm)));
    vm_pop_prev(vm);
}

void rkl_add(VM *vm) {
    int result_value = 0;

    LispNode *list = vm_peek(vm);
    while (list->kind == LISP_CONS) {
        result_value += CAR(list)->as.integer;
        list = CDR(list);
    }
    vm_pop(vm);
    vm_build_integer(vm, result_value);
}

void rkl_gt(VM *vm) {
    int value2 = vm_peek(vm)->as.integer;
    vm_pop(vm);
    int value1 = vm_peek(vm)->as.integer;
    vm_pop(vm);

    if (value1 > value2)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_is_nil(VM *vm) {
    bool is_nil = vm_peek(vm)->kind == LISP_NIL;
    vm_pop(vm);

    if (is_nil)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_eval(VM *vm) {
    vm_eval_expr(vm);
}

void rkl_list(VM *vm) {
    ASSERT_HAS(vm, 1);
    (void)vm;
}

const BuiltinDef BUILTINS[] = {
    {"+", {rkl_add, 0, true}},       {"-", {rkl_sub, 2, false}},
    {"=", {rkl_eq, 2, false}},       {">", {rkl_gt, 2, false}},
    {"print", {rkl_print, 0, true}}, {"cons", {rkl_cons, 2, false}},
    {"car", {rkl_car, 1, false}},    {"cdr", {rkl_cdr, 1, false}},
    {"eval", {rkl_eval, 1, false}},  {"nil?", {rkl_is_nil, 1, false}},
    {"list", {rkl_list, 0, true}},
};

const size_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(BUILTINS[0]);
