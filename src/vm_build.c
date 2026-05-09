#include "gc.h"
#include "vm.h"

void vm_build_value(VM *vm, LispNodeKind kind) {
    if (gc_check_bounds(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
    vm_push(vm, gc_alloc_node(vm->gc, kind));
}

void vm_build_integer(VM *vm, Integer value) {
    vm_build_value(vm, LISP_INTEGER);
    INTEGER(vm_peek(vm)) = value;
}

void vm_build_bool(VM *vm, bool value) {
    vm_build_value(vm, LISP_BOOL);
    BOOL(vm_peek(vm)) = value;
}

void vm_build_float(VM *vm, double value) {
    vm_build_value(vm, LISP_FLOAT);
    FLOAT(vm_peek(vm)) = value;
}

void vm_build_exception(VM *vm, ExceptionKind exception) {
    vm_build_value(vm, LISP_EXCEPTION);
    EXCEPTION(vm_peek(vm)) = exception;
}

void vm_build_builtin(VM *vm, LispBuiltin value) {
    vm_build_value(vm, LISP_BUILTIN);
    BUILTIN(vm_peek(vm)) = value;
}

void vm_build_nil(VM *vm) {
    vm_build_value(vm, LISP_NIL);
}

void vm_build_lambda(VM *vm, LambdaArgs args, bool is_variadic, LispNode *expr, Scope *scope) {
    vm_build_value(vm, LISP_LAMBDA);
    LAMBDA(vm_peek(vm)).args = args;
    LAMBDA(vm_peek(vm)).is_variadic = is_variadic;
    LAMBDA(vm_peek(vm)).subexpr = expr;
    LAMBDA(vm_peek(vm)).scope = scope;
}
