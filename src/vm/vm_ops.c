#include "object.h"
#include "vm.h"
#include <assert.h>

// t, h -> (h . t)
void vm_pack_cons(VM *vm) {
    vm_build_cons(vm, vm_peek(vm), vm_prev(vm));
    vm_pop_prev_n(vm, 2);
}

// (h . t) -> t, h
void vm_unpack_cons(VM *vm) {
    ASSERT_KIND(vm, LISP_CONS);

    vm_push_prev(vm, CDR(vm_peek(vm)));
    vm_push_prev(vm, CAR(vm_peek(vm)));
    vm_pop(vm);
}

// [x] -> x * n
size_t vm_unpack_list(VM *vm) {
    size_t size = 0;
    while (OFTYPE(vm_peek(vm), TY_CONS)) {
        vm_unpack_cons(vm);
        vm_swap(vm);

        size++;
    }

    assert(OFTYPE(vm_peek(vm), TY_NIL));
    vm_pop(vm);
    return size;
}
