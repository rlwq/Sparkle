#include "builtins.h"
#include "object.h"
#include "vm.h"

// Value -> Value
static void spk_eval(VM *vm) {
    vm_eval_object(vm);
}

// Symbol -> (does not return)
static void spk_throw(VM *vm) {
    vm_expect(vm, TY_SYMBOL);
    vm_recover(vm, vm_peek(vm));
}

// Callable, List (args) -> Value
static void spk_apply(VM *vm) {
    vm_expect(vm, TY_LIST);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_prev(vm), TY_CALLABLE), vm->singletons._UNCALLABLE_EXCEPTION);

    vm_swap(vm);
    vm_call(vm);
}

DEFINE_MODULE(CONTROL) = {
    {"eval", spk_eval, 1, false},
    {"throw", spk_throw, 1, false},
    {"apply", spk_apply, 2, false},
};

DEFINE_MODULE_SIZE(CONTROL);
