#include "vm.h"

// 1 -> 2
void vm_dup(VM *vm) {
    ASSERT_HAS(vm, 1);
    da_push(vm->value_stack, VM_STACK_AT(vm, 0));
}

// 2 -> 3
void vm_dup_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    da_push(vm->value_stack, VM_STACK_AT(vm, 1));
}

// 0 -> 1
void vm_push(VM *vm, LispNode *value) {
    assert(value);
    da_push(vm->value_stack, value);
}

// 1 -> 2
void vm_push_prev(VM *vm, LispNode *value) {
    assert(value);
    // TODO: rewrite
    vm_push(vm, value);
    vm_swap(vm);
}

// Node ->
void vm_pop(VM *vm) {
    ASSERT_HAS(vm, 1);
    da_pop(vm->value_stack);
}

// Node * n ->
void vm_pop_n(VM *vm, size_t n) {
    ASSERT_HAS(vm, n);
    vm->value_stack.size -= n;
}

// Node (x), Node (y) -> Node (y)
void vm_pop_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    VM_STACK_AT(vm, 1) = VM_STACK_AT(vm, 0);
    da_pop(vm->value_stack);
}

// Node (x) * n, Node (y) -> Node (y)
void vm_pop_prev_n(VM *vm, size_t n) {
    ASSERT_HAS(vm, n + 1);
    VM_STACK_AT(vm, n) = VM_STACK_AT(vm, 0);
    vm->value_stack.size -= n;
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
