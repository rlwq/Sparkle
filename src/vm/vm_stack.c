#include "vm.h"

#define STACK_AT(e_, n_) (da_at_end((e_)->value_stack, (n_)))
#define STACK_PUSH(e_, v_) da_push((e_)->value_stack, (v_))
#define ASSERT_HAS(e_, n_) (assert((e_)->value_stack.size >= (n_)))

void vm_push(VM *vm, Object *value) {
    assert(value);
    da_push(vm->value_stack, value);
}

// a, b -> a, INSERTED, b
void vm_push_prev(VM *vm, Object *value) {
    assert(value);
    STACK_PUSH(vm, STACK_AT(vm, 0));
    STACK_AT(vm, 1) = value;
}

// x
Object *vm_peek(VM *vm) {
    ASSERT_HAS(vm, 1);
    return STACK_AT(vm, 0);
}

// x, y
Object *vm_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    return STACK_AT(vm, 1);
}

// a -> a, a
void vm_dup(VM *vm) {
    ASSERT_HAS(vm, 1);
    STACK_PUSH(vm, STACK_AT(vm, 0));
}

// a, b -> a, b, a
void vm_dup_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    STACK_PUSH(vm, STACK_AT(vm, 1));
}

// x, y -> y, x
void vm_swap(VM *vm) {
    ASSERT_HAS(vm, 2);
    Object *curr = STACK_AT(vm, 0);
    STACK_AT(vm, 0) = STACK_AT(vm, 1);
    STACK_AT(vm, 1) = curr;
}

// x, y, z -> y, z, x
void vm_rot(VM *vm) {
    ASSERT_HAS(vm, 3);
    Object *curr = STACK_AT(vm, 0);
    STACK_AT(vm, 0) = STACK_AT(vm, 2);
    STACK_AT(vm, 2) = STACK_AT(vm, 1);
    STACK_AT(vm, 1) = curr;
}

// a ->
void vm_pop(VM *vm) {
    ASSERT_HAS(vm, 1);
    da_pop(vm->value_stack);
}

// a * n ->
void vm_pop_n(VM *vm, size_t n) {
    ASSERT_HAS(vm, n);
    vm->value_stack.size -= n;
}

// x, y -> x
void vm_pop_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    STACK_AT(vm, 1) = STACK_AT(vm, 0);
    da_pop(vm->value_stack);
}

// x * n, y -> y
void vm_pop_prev_n(VM *vm, size_t n) {
    ASSERT_HAS(vm, n + 1);
    STACK_AT(vm, n) = STACK_AT(vm, 0);
    vm->value_stack.size -= n;
}
