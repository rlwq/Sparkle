#include "vm.h"

// 0 -> 1
void vm_push(VM *vm, LispNode *value) {
    assert(value);
    da_push(vm->value_stack, value);
}

// Node ->
void vm_pop(VM *vm) {
    ASSERT_HAS(vm, 1);
    assert(vm->value_stack.size > 0);
    da_pop(vm->value_stack);
}

// Node (x), Node (y) -> Node (y)
void vm_pop_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    VM_STACK_AT(vm, 1) = VM_STACK_AT(vm, 0);
    da_pop(vm->value_stack);
}

// Node (x), Node (y) -> Node (y), Node (x)
void vm_swap(VM *vm) {
    ASSERT_HAS(vm, 2);
    LispNode *curr = VM_STACK_AT(vm, 0);
    VM_STACK_AT(vm, 0) = VM_STACK_AT(vm, 1);
    VM_STACK_AT(vm, 1) = curr;
}

// Node (x), Node (y), Node (z) -> Node (y), Node (z), Node (x)
void vm_rot(VM *vm) {
    ASSERT_HAS(vm, 3);
    LispNode *curr = VM_STACK_AT(vm, 0);
    VM_STACK_AT(vm, 0) = VM_STACK_AT(vm, 2);
    VM_STACK_AT(vm, 2) = VM_STACK_AT(vm, 1);
    VM_STACK_AT(vm, 1) = curr;
}

// Node (x), Node * pos
LispNode *vm_stack_at(VM *vm, size_t pos) {
    ASSERT_HAS(vm, pos + 1);
    return VM_STACK_AT(vm, pos);
}

// Node
LispNode *vm_peek(VM *vm) {
    ASSERT_HAS(vm, 1);
    return VM_STACK_AT(vm, 0);
}

// Node, Node
LispNode *vm_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    return VM_STACK_AT(vm, 1);
}
