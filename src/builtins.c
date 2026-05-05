#include <stdio.h>

#include "builtins.h"
#include "debug.h"
#include "forwards.h"
#include "lisp_node.h"
#include "vm.h"

const BuiltinDef BUILTINS[] = {
    {"+", {rkl_add, 0, true}},       {"-", {rkl_sub, 2, false}},
    {"=", {rkl_int_eq, 2, false}},   {">", {rkl_gt, 2, false}},
    {"print", {rkl_print, 0, true}}, {"cons", {rkl_cons, 2, false}},
    {"car", {rkl_car, 1, false}},    {"cdr", {rkl_cdr, 1, false}},
    {"eval", {rkl_eval, 1, false}},  {"nil?", {rkl_is_nil, 1, false}},
};

const size_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(BUILTINS[0]);

void rkl_int_eq(VM *vm) {
    int value2 = vm_peek(vm)->as.integer;
    vm_pop(vm);
    int value1 = vm_peek(vm)->as.integer;
    vm_pop(vm);

    if (value1 == value2)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_sub(VM *vm) {
    int value2 = vm_peek(vm)->as.integer;
    vm_pop(vm);
    int value1 = vm_peek(vm)->as.integer;
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

void rkl_cons(VM *vm) {
    vm_build_value(vm, LISP_CONS);
    vm_peek(vm)->as.cons.cdr = da_at_end(vm->value_stack, 1);
    vm_peek(vm)->as.cons.car = da_at_end(vm->value_stack, 2);
    vm_pop_prev(vm);
    vm_pop_prev(vm);
}

void rkl_car(VM *vm) {
    vm_push(vm, CAR(vm_peek(vm)));
    vm_pop_prev(vm);
}

void rkl_cdr(VM *vm) {
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
