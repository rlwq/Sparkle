#include <stdio.h>

#include "builtins.h"
#include "debug.h"
#include "vm.h"

const BuiltinDef BUILTINS[] = {
    {"+", rkl_add},       {"-", rkl_sub},       {"=", rkl_int_eq}, {">", rkl_gt},
    {"print", rkl_print}, {"cons", rkl_cons},   {"car", rkl_car},  {"cdr", rkl_cdr},
    {"eval", rkl_eval},   {"nil?", rkl_is_nil},
};

const size_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(BUILTINS[0]);

void rkl_int_eq(VM *vm, size_t args_count) {
    assert(args_count == 2);
    int value2 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    int value1 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);

    if (value1 == value2)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_sub(VM *vm, size_t args_count) {
    assert(args_count == 2);
    int value2 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    int value1 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);

    vm_build_integer(vm, value1 - value2);
}

void rkl_print(VM *vm, size_t args_count) {
    for (; args_count > 0; args_count--) {
        LispNode *expr = vm_peek_value(vm);
        print_expr(expr);
        vm_pop_value(vm);
        printf("\n");
    }
    vm_build_nil(vm);
}

void rkl_cons(VM *vm, size_t args_count) {
    assert(args_count == 2);
    vm_build_value(vm, LISP_CONS);
    vm_peek_value(vm)->as.cons.cdr = da_at_end(vm->value_stack, 1);
    vm_peek_value(vm)->as.cons.car = da_at_end(vm->value_stack, 2);
    vm_pop_prev_value(vm);
    vm_pop_prev_value(vm);
}

void rkl_car(VM *vm, size_t args_count) {
    assert(args_count == 1);
    vm_push_value(vm, CAR(vm_peek_value(vm)));
    vm_pop_prev_value(vm);
}

void rkl_cdr(VM *vm, size_t args_count) {
    assert(args_count == 1);
    vm_push_value(vm, CDR(vm_peek_value(vm)));
    vm_pop_prev_value(vm);
}

void rkl_add(VM *vm, size_t args_count) {
    int result_value = 0;

    for (size_t i = 0; i < args_count; i++) {
        LispNode *popped = vm_peek_value(vm);
        result_value += popped->as.integer;
        vm_pop_value(vm);
    }
    vm_build_integer(vm, result_value);
}

void rkl_gt(VM *vm, size_t args_count) {
    assert(args_count == 2);
    int value2 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    int value1 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);

    if (value1 > value2)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_is_nil(VM *vm, size_t args_count) {
    assert(args_count == 1);

    bool is_nil = vm_peek_value(vm)->kind == LISP_NIL;
    vm_pop_value(vm);

    if (is_nil)
        vm_build_integer(vm, 1);
    else
        vm_build_nil(vm);
}

void rkl_eval(VM *vm, size_t args_count) {
    assert(args_count == 1);
    vm_eval_expr(vm);
}
