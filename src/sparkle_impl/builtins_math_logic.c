#include <stdbool.h>

#include "builtins.h"
#include "forwards.h"
#include "lisp_node.h"
#include "utils.h"
#include "vm.h"

#define NUMERIC_VARIADIC_BUILTIN(func_name_, operator_)                                            \
    void func_name_(VM *vm) {                                                                      \
        ASSERT_HAS(vm, 2);                                                                         \
        LispNode *list = vm_peek(vm);                                                              \
        vm_swap(vm);                                                                               \
                                                                                                   \
        while (list->kind == LISP_CONS) {                                                          \
            vm_push(vm, CAR(list));                                                                \
            vm_to_common_numeric(vm);                                                              \
                                                                                                   \
            if (vm_peek(vm)->kind == LISP_BOOL)                                                    \
                vm_build_integer(vm,                                                               \
                                 (Integer)BOOL(vm_prev(vm)) operator_(Integer) BOOL(vm_peek(vm))); \
            else if (vm_peek(vm)->kind == LISP_INTEGER)                                            \
                vm_build_integer(vm, INTEGER(vm_prev(vm)) operator_ INTEGER(vm_peek(vm)));         \
            else if (vm_peek(vm)->kind == LISP_FLOAT)                                              \
                vm_build_float(vm, FLOAT(vm_prev(vm)) operator_ FLOAT(vm_peek(vm)));               \
                                                                                                   \
            vm_pop_prev_n(vm, 2);                                                                  \
                                                                                                   \
            list = CDR(list);                                                                      \
        }                                                                                          \
        vm_pop_prev(vm);                                                                           \
    }

#define NUMERIC_ORDER_BUILTIN(func_name_, operator_)                                               \
    void func_name_(VM *vm) {                                                                      \
        ASSERT_HAS(vm, 2);                                                                         \
        vm_to_common_numeric(vm);                                                                  \
        if (vm_peek(vm)->kind == LISP_BOOL)                                                        \
            vm_build_bool(vm, BOOL(vm_prev(vm)) operator_ BOOL(vm_peek(vm)));                      \
        else if (vm_peek(vm)->kind == LISP_INTEGER)                                                \
            vm_build_bool(vm, INTEGER(vm_prev(vm)) operator_ INTEGER(vm_peek(vm)));                \
        else if (vm_peek(vm)->kind == LISP_FLOAT)                                                  \
            vm_build_bool(vm, FLOAT(vm_prev(vm)) operator_ FLOAT(vm_peek(vm)));                    \
        vm_pop_prev_n(vm, 2);                                                                      \
    }

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
    vm_pop_n(vm, 2);
    vm_build_bool(vm, is_equal);
}

void rkl_ne(VM *vm) {
    ASSERT_HAS(vm, 2);
    rkl_eq(vm);
    vm_build_bool(vm, !BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

NUMERIC_VARIADIC_BUILTIN(rkl_add, +)
NUMERIC_VARIADIC_BUILTIN(rkl_mul, *)
NUMERIC_VARIADIC_BUILTIN(rkl_sub, -)

NUMERIC_ORDER_BUILTIN(rkl_gt, >)
NUMERIC_ORDER_BUILTIN(rkl_ge, >=)
NUMERIC_ORDER_BUILTIN(rkl_lt, <)
NUMERIC_ORDER_BUILTIN(rkl_le, <=)

void rkl_truediv(VM *vm) {
    ASSERT_HAS(vm, 2);

    if (!IS_NUMBERIC(vm_peek(vm)) || !IS_NUMBERIC(vm_prev(vm)))
        vm_recover(vm, WRONG_TYPE);

    vm_to_common_numeric(vm);

    if (vm_peek(vm)->kind == LISP_BOOL)
        vm_build_float(vm, (double)BOOL(vm_prev(vm)) / (double)BOOL(vm_peek(vm)));
    else if (vm_peek(vm)->kind == LISP_INTEGER)
        vm_build_float(vm, (double)INTEGER(vm_prev(vm)) / (double)INTEGER(vm_peek(vm)));
    else if (vm_peek(vm)->kind == LISP_FLOAT)
        vm_build_float(vm, FLOAT(vm_prev(vm)) / FLOAT(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

void rkl_div(VM *vm) {
    ASSERT_HAS(vm, 2);

    if (vm_peek(vm)->kind != LISP_INTEGER || vm_prev(vm)->kind != LISP_INTEGER)
        vm_recover(vm, WRONG_TYPE);

    VM_RECOVER_IF(vm, INTEGER(vm_peek(vm)) == 0, WRONG_TYPE);

    vm_build_integer(vm, INTEGER(vm_prev(vm)) / INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

void rkl_mod(VM *vm) {
    ASSERT_HAS(vm, 2);

    VM_RECOVER_IF(vm, !NODE_IS(vm_peek(vm), LISP_INTEGER), WRONG_TYPE);
    VM_RECOVER_IF(vm, !NODE_IS(vm_prev(vm), LISP_INTEGER), WRONG_TYPE);
    VM_RECOVER_IF(vm, INTEGER(vm_peek(vm)) == 0, WRONG_VALUE);

    vm_build_integer(vm, INTEGER(vm_prev(vm)) % INTEGER(vm_peek(vm)));
    vm_pop_prev_n(vm, 2);
}

void rkl_eval(VM *vm) {
    ASSERT_HAS(vm, 1);
    vm_eval_node(vm);
}

void rkl_is_nil(VM *vm) {
    ASSERT_HAS(vm, 1);
    bool is_nil = vm_peek(vm)->kind == LISP_NIL;
    vm_pop(vm);
    vm_build_bool(vm, is_nil);
}

void rkl_cast_to_bool(VM *vm) {
    ASSERT_HAS(vm, 1);
    vm_cast_to_bool(vm);
}

void rkl_logical_and(VM *vm) {
    ASSERT_HAS(vm, 1);
    bool result = true;

    for (LispNode *curr = vm_peek(vm); !NODE_IS(curr, LISP_NIL); curr = CDR(curr)) {
        vm_push(vm, CAR(curr));
        vm_cast_to_bool(vm);
        result = result && BOOL(vm_peek(vm));
        if (!result)
            break;
        vm_pop(vm);
    }

    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_logical_or(VM *vm) {
    ASSERT_HAS(vm, 1);
    bool result = false;

    for (LispNode *curr = vm_peek(vm); !NODE_IS(curr, LISP_NIL); curr = CDR(curr)) {
        vm_push(vm, CAR(curr));
        vm_cast_to_bool(vm);
        result = result || BOOL(vm_peek(vm));
        if (result)
            break;
        vm_pop(vm);
    }

    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_logical_not(VM *vm) {
    ASSERT_HAS(vm, 1);
    vm_cast_to_bool(vm);
    vm_build_bool(vm, !BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

DEFINE_MODULE(MATH_LOGIC) = {
    {"+", rkl_add, 1, true},           {"*", rkl_mul, 1, true},
    {"-", rkl_sub, 1, true},           {"/", rkl_truediv, 2, false},
    {"=", rkl_eq, 2, false},           {"!=", rkl_ne, 2, false},
    {">", rkl_gt, 2, false},           {">=", rkl_ge, 2, false},
    {"<", rkl_lt, 2, false},           {"<=", rkl_le, 2, false},
    {"div", rkl_div, 2, false},        {"mod", rkl_mod, 2, false},
    {"eval", rkl_eval, 1, false},      {"nil?", rkl_is_nil, 1, false},
    {"?", rkl_cast_to_bool, 1, false}, {"not", rkl_logical_not, 1, false},
    {"&&", rkl_logical_and, 0, true},  {"||", rkl_logical_or, 0, true},
};

DEFINE_MODULE_SIZE(MATH_LOGIC);
