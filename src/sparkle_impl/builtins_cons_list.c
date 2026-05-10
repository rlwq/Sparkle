#include <stdbool.h>

#include "builtins.h"
#include "forwards.h"
#include "lisp_node.h"
#include "vm.h"

// Node, Node -> Cons
void rkl_cons(VM *vm) {
    ASSERT_HAS(vm, 2);

    vm_build_value(vm, LISP_CONS);
    CONS(vm_peek(vm)).cdr = vm_prev(vm);
    vm_pop_prev(vm);
    CONS(vm_peek(vm)).car = vm_prev(vm);
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

void rkl_list(VM *vm) {
    ASSERT_HAS(vm, 1);
    (void)vm;
}

DEFINE_MODULE(CONS_LIST) = {
    {"cons", rkl_cons, 2, false},
    {"car", rkl_car, 1, false},
    {"cdr", rkl_cdr, 1, false},
    {"list", rkl_list, 0, true},
};

DEFINE_MODULE_SIZE(CONS_LIST);
