#include <stdbool.h>

#include "builtins.h"
#include "forwards.h"
#include "object.h"
#include "vm.h"

// Node, Node -> Cons
void rkl_cons(VM *vm) {
    vm_swap(vm);
    vm_pack_cons(vm);
}

// Cons -> Node
void rkl_car(VM *vm) {
    vm_expect(vm, TY_CONS);

    vm_push(vm, CAR(vm_peek(vm)));
    vm_pop_prev(vm);
}

// Cons -> Node
void rkl_cdr(VM *vm) {
    vm_expect(vm, TY_CONS);

    vm_push(vm, CDR(vm_peek(vm)));
    vm_pop_prev(vm);
}

void rkl_list(VM *vm) {
    (void)vm;
}

void rkl_len(VM *vm) {
    vm_expect(vm, TY_LISTFUL);

    Integer size = 0;

    LIST_ITER(vm, curr, vm_peek(vm))
        size++;
    END_LIST_ITER(vm, curr)

    vm_pop(vm);
    vm_build_integer(vm, size);
}

DEFINE_MODULE(CONS_LIST) = {
    {"cons", rkl_cons, 2, false}, {"car", rkl_car, 1, false}, {"cdr", rkl_cdr, 1, false},
    {"list", rkl_list, 0, true},  {"len", rkl_len, 1, false},
};

DEFINE_MODULE_SIZE(CONS_LIST);
