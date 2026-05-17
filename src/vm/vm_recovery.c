#include "string_interner.h"
#include "vm.h"

void vm_push_recovery(VM *vm, jmp_buf *jmp) {
    RecoveryStackEntry recovery = (RecoveryStackEntry){
        .jmp = jmp, .scopes_count = vm->scope_stack.size, .values_count = vm->value_stack.size};
    da_push(vm->recovery_stack, recovery);
}

void vm_pop_recovery(VM *vm) {
    assert(vm->recovery_stack.size > 0);
    da_pop(vm->recovery_stack);
}

void vm_recover(VM *vm, StringName value) {
    assert(vm->recovery_stack.size > 0);

    RecoveryStackEntry recovery = da_at_end(vm->recovery_stack, 0);

    vm->value_stack.size = recovery.values_count;
    vm->scope_stack.size = recovery.scopes_count;
    vm->exception = value;

    longjmp(*(recovery.jmp), 1);
}

void vm_expect(VM *vm, ObjectType type) {
    VM_RECOVER_IF(vm, !(OFTYPE(vm_peek(vm), type)), vm->si->prebuilt._TYPE_EXCEPTION);
}

void vm_expect2(VM *vm, ObjectType prev, ObjectType peek) {
    VM_RECOVER_IF(vm, !(OFTYPE(vm_peek(vm), peek)), vm->si->prebuilt._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !(OFTYPE(vm_prev(vm), prev)), vm->si->prebuilt._TYPE_EXCEPTION);
}
