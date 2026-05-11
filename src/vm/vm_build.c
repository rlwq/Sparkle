#include "gc.h"
#include "vm.h"

void vm_build_value(VM *vm, ObjectKind kind) {
    if (gc_check_bounds(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
    vm_push(vm, gc_alloc_node(vm->gc, kind));
}

void vm_build_integer(VM *vm, Integer value) {
    vm_build_value(vm, KIND_INTEGER);
    INTEGER(vm_peek(vm)) = value;
}

void vm_build_symbol(VM *vm, StringName value) {
    vm_build_value(vm, KIND_SYMBOL);
    SYMBOL(vm_peek(vm)) = value;
}

void vm_build_cons(VM *vm, Object *car, Object *cdr) {
    vm_build_value(vm, KIND_CONS);
    CAR(vm_peek(vm)) = car;
    CDR(vm_peek(vm)) = cdr;
}

void vm_build_bool(VM *vm, bool value) {
    if (value)
        vm_push(vm, vm->singletons._True);
    else
        vm_push(vm, vm->singletons._False);
}

void vm_build_float(VM *vm, double value) {
    vm_build_value(vm, KIND_FLOAT);
    FLOAT(vm_peek(vm)) = value;
}

void vm_build_exception(VM *vm, ExceptionKind exception) {
    vm_build_value(vm, KIND_EXCEPTION);
    EXCEPTION(vm_peek(vm)) = exception;
}

void vm_build_builtin(VM *vm, BuiltinObject value) {
    vm_build_value(vm, KIND_BUILTIN);
    BUILTIN(vm_peek(vm)) = value;
}

void vm_build_nil(VM *vm) {
    vm_push(vm, vm->singletons._Nil);
}

void vm_build_lambda(VM *vm, LambdaArgs args, bool is_variadic, Object *expr, Scope *scope) {
    vm_build_value(vm, KIND_LAMBDA);
    LAMBDA(vm_peek(vm)).args = args;
    LAMBDA(vm_peek(vm)).is_variadic = is_variadic;
    LAMBDA(vm_peek(vm)).subexpr = expr;
    LAMBDA(vm_peek(vm)).scope = scope;
}
