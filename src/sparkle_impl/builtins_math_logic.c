#include <stdbool.h>

#include "builtins.h"
#include "forwards.h"
#include "lisp_node.h"
#include "utils.h"
#include "vm.h"

// Node, Node -> Node
void rkl_eq(VM *vm) {
    ASSERT_HAS(vm, 2);

    bool is_equal = false;

    if (IS_NUMBERIC(vm_peek(vm)) && IS_NUMBERIC(vm_prev(vm)))
        vm_to_common_numeric(vm);

    if (vm_peek(vm)->kind != vm_prev(vm)->kind) {
        is_equal = false;
        goto end;
    }

    switch (vm_peek(vm)->kind) {
    case LISP_NIL:
        is_equal = true;
        break;
    case LISP_SYMBOL:
        is_equal = SYMBOL(vm_peek(vm)) == SYMBOL(vm_prev(vm));
        break;
    case LISP_INTEGER:
        is_equal = INTEGER(vm_peek(vm)) == INTEGER(vm_prev(vm));
        break;
    case LISP_EXCEPTION:
        is_equal = EXCEPTION(vm_peek(vm)) == EXCEPTION(vm_prev(vm));
        break;
    case LISP_FLOAT:
        is_equal = FLOAT(vm_peek(vm)) == FLOAT(vm_prev(vm));
        break;
    case LISP_BOOL:
        is_equal = BOOL(vm_peek(vm)) == BOOL(vm_prev(vm));
        break;
    case LISP_CONS:
        is_equal = vm_peek(vm) == vm_prev(vm);
        break;
    case LISP_BUILTIN:
    case LISP_LAMBDA:
    case LISP_STRING:
        NOT_IMPLEMENTED();
        break;
    }

end:
    vm_pop(vm);
    vm_pop(vm);
    vm_build_bool(vm, is_equal);
}

// Integer, Integer -> Integer
void rkl_sub(VM *vm) {
    ASSERT_HAS(vm, 2);

    vm_expect_kind(vm, LISP_INTEGER, WRONG_TYPE);
    Integer value2 = INTEGER(vm_peek(vm));
    vm_pop(vm);

    vm_expect_kind(vm, LISP_INTEGER, WRONG_TYPE);
    Integer value1 = INTEGER(vm_peek(vm));
    vm_pop(vm);

    vm_build_integer(vm, value1 - value2);
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
    int value2 = INTEGER(vm_peek(vm));
    vm_pop(vm);
    int value1 = INTEGER(vm_peek(vm));
    vm_pop(vm);
    vm_build_bool(vm, value1 > value2);
}

void rkl_eval(VM *vm) {
    vm_eval_node(vm);
}

void rkl_is_nil(VM *vm) {
    bool is_nil = vm_peek(vm)->kind == LISP_NIL;
    vm_pop(vm);
    vm_build_bool(vm, is_nil);
}

DEFINE_MODULE(MATH_LOGIC) = {{"+", {rkl_add, 0, true}},      {"-", {rkl_sub, 2, false}},
                             {"=", {rkl_eq, 2, false}},      {">", {rkl_gt, 2, false}},
                             {"eval", {rkl_eval, 1, false}}, {"nil?", {rkl_is_nil, 1, false}}};
DEFINE_MODULE_SIZE(MATH_LOGIC);
