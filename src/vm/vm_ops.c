#include "object.h"
#include "vm.h"

#include <assert.h>

// v0, v1, ..., v[n-1] -> List(v0, v1, ..., v[n-1])
void vm_pack_list(VM *vm, size_t length) {
    vm_build_list(vm);
    Object *list = vm_peek(vm);

    for (size_t i = length; i >= 1; i--)
        da_push(OBJ_LIST_ITEMS(list), da_at_end(vm->value_stack, i));

    vm_pop_prev_n(vm, length);
}

// List(v0, v1, ..., v[n-1]) -> v0, v1, ..., v[n-1]
size_t vm_unpack_list(VM *vm) {
    ASSERT_KIND(vm, KIND_LIST);

    Object *list = vm_peek(vm);
    size_t size = OBJ_LIST_SIZE(list);

    vm_pop(vm);

    for (size_t i = 0; i < size; i++)
        vm_push(vm, OBJ_LIST_AT(list, i));

    return size;
}
