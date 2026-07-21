#include "builtins.h"
#include "object.h"
#include "vm.h"

static void spk_eval(VM *vm) {
    vm_eval_object(vm);
}

// Symbol -> does not return. A plain function rather than a special form: the
// kind has to be evaluated anyway, and a builtin is handed its arguments
// already evaluated, so there is nothing for a form to control here.
//
// The kind is a Symbol because that is what an exception is in this language -
// try matches on it and the reporter prints it.
static void spk_throw(VM *vm) {
    vm_expect(vm, TY_SYMBOL);
    vm_recover(vm, vm_peek(vm));
}

// List (args), Callable -> Object. vm_call is the primitive that invokes a
// callable over an argument list without evaluating the arguments again, which
// is exactly what apply needs.
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
