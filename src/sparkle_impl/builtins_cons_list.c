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

// Cons, Value -> Cons
void rkl_setcar(VM *vm) {
    vm_expect2(vm, TY_CONS, TY_ANY);
    CAR(vm_prev(vm)) = vm_peek(vm);
    vm_pop(vm);
}

// Cons, Value -> Cons
void rkl_setcdr(VM *vm) {
    vm_expect2(vm, TY_CONS, TY_ANY);
    CDR(vm_prev(vm)) = vm_peek(vm);
    vm_pop(vm);
}

void rkl_list(VM *vm) {
    /* The args. list is already on the stack. Does nothing */
    (void)vm;
}

void rkl_len(VM *vm) {
    vm_expect(vm, TY_LISTFUL);

    Integer size = 0;

    LIST_ITER(vm, curr, vm_peek(vm))
        size++;
    END_LIST_ITER_RECOVER(vm, curr)

    vm_pop(vm);
    vm_build_integer(vm, size);
}

// Callable, List -> List
void rkl_map(VM *vm) {
    vm_expect2(vm, TY_CALLABLE, TY_LISTFUL);

    Object *callable = vm_prev(vm);

    size_t length = 1;
    LIST_ITER(vm, curr, vm_peek(vm))
        vm_push(vm, callable);
        vm_push(vm, CAR(curr));
        vm_build_nil(vm);
        vm_pack_list(vm, 3);
        vm_eval_cons(vm);
        length++;
    END_LIST_ITER_RECOVER(vm, curr)

    vm_build_nil(vm);
    vm_pack_list(vm, length);

    vm_pop_prev_n(vm, 2);
}

// Callable, List -> List
void rkl_filter(VM *vm) {
    vm_expect2(vm, TY_CALLABLE, TY_LISTFUL);

    Object *callable = vm_prev(vm);

    size_t length = 1;
    LIST_ITER(vm, curr, vm_peek(vm))
        vm_push(vm, callable);
        vm_push(vm, CAR(curr));
        vm_build_nil(vm);
        vm_pack_list(vm, 3);
        vm_eval_cons(vm);
        vm_cast_to_bool(vm);
        bool result = BOOL(vm_peek(vm));
        vm_pop(vm);
        if (result) {
            vm_push(vm, CAR(curr));
            length++;
        }
    END_LIST_ITER_RECOVER(vm, curr)

    vm_build_nil(vm);
    vm_pack_list(vm, length);

    vm_pop_prev_n(vm, 2);
}

// Integer, List -> List
void rkl_take(VM *vm) {
    vm_expect2(vm, TY_INTEGER, TY_LISTFUL);

    size_t length = INTEGER(vm_prev(vm));

    Object *node = vm_peek(vm);
    LIST_ITER(vm, curr, vm_peek(vm))
    END_LIST_ITER_RECOVER(vm, curr)
}

DEFINE_MODULE(CONS_LIST) = {
    {"cons", rkl_cons, 2, false},     {"car", rkl_car, 1, false},
    {"cdr", rkl_cdr, 1, false},       {"setcar", rkl_setcar, 2, false},
    {"setcdr", rkl_setcdr, 2, false}, {"list", rkl_list, 0, true},
    {"len", rkl_len, 1, false},       {"map", rkl_map, 2, false},
    {"filter", rkl_filter, 2, false},
};

DEFINE_MODULE_SIZE(CONS_LIST);
