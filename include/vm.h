#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "object.h"
#include "string_interner.h"
#include <setjmp.h>

#define VM_CURR_SCOPE(e_) (da_at_end((e_)->scope_stack, 0))

#define ASSERT_KIND(e_, k_)                                                                        \
    (assert(((e_)->value_stack.size > 0) && (da_at_end((e_)->value_stack, 0)->kind == (k_))))

#define VM_RECOVER_IF(vm_, expr_, ex_)                                                             \
    if ((expr_))                                                                                   \
        vm_recover(vm_, ex_);

// Singletons are allocated once in vm_alloc and never again: vm_build_bool and
// vm_build_nil push these same objects, so comparing pointers against a
// singleton is an identity check.
#define X_RUNTIME_SINGLETONS                                                                       \
    X(Nil, KIND_NIL, {0})                                                                          \
    X(True, KIND_BOOL, {.bool_ = true})                                                            \
    X(False, KIND_BOOL, {.bool_ = false})

#define X_RUNTIME_EXCEPTIONS                                                                       \
    X(TYPE_EXCEPTION, "Function expected some other object type.")                                 \
    X(ARITY_EXCEPTION, "Wrong arity for a function call.")                                         \
    X(UNDEFINED_EXCEPTION, "Symbol has no definition.")                                            \
    X(REBINDING_EXCEPTION, "Symbol is already bound.")                                             \
    X(UNCALLABLE_EXCEPTION, "Can't use this kind of object as a function.")                        \
    X(VALUE_EXCEPTION, "Function expected some other value.")

typedef struct {
    jmp_buf *jmp;
    size_t values_count;
    size_t scopes_count;
} RecoveryStackEntry;

struct VM {
    ObjectPtrDA instructions;
    size_t instruction_ptr;

    DA(Scope *) scope_stack;
    DA(Object *) value_stack;
    DA(RecoveryStackEntry) recovery_stack;

    GC *gc;
    StringInterner *si;

    // Special-form dispatch, injected by special_forms_attach so the VM core
    // never names the language layer. The evaluator consults it before
    // treating a list head as a function call; NULL means no forms installed.
    bool (*try_special)(VM *vm, StringName name);

    struct {
#define X(name_, kind_, init_) Object *_##name_;
        X_RUNTIME_SINGLETONS
#undef X

#define X(name_, msg_) Object *_##name_;
        X_RUNTIME_EXCEPTIONS
#undef X
    } singletons;

    Object *exception;
    bool is_err;
};

// vm.c
VM *vm_alloc(GC *gc, StringInterner *si);
void vm_load_instructions(VM *vm, ObjectPtrDA instructions);
void vm_free(VM *vm);
void vm_mark(VM *vm);
void vm_register_builtin(VM *vm, const char *name, BuiltinObject func_ptr);
void vm_run(VM *vm);

// vm_recovery.c
void vm_push_recovery(VM *vm, jmp_buf *jmp);
void vm_pop_recovery(VM *vm);
void vm_recover(VM *vm, Object *exception_symbol) __attribute__((noreturn)) __attribute__((cold));
void vm_expect(VM *vm, ObjectType type) __attribute__((cold));
void vm_expect2(VM *vm, ObjectType prev, ObjectType peek) __attribute__((cold));

// vm_scope.c
void vm_build_scope(VM *vm);
void vm_push_scope(VM *vm, Scope *scope);
void vm_pop_scope(VM *vm);
void vm_scope_define(VM *vm, StringName name);
void vm_scope_get(VM *vm, StringName name);
void vm_scope_set(VM *vm, StringName name);

// vm_build.c
//
// Constructors are collect-then-alloc-then-push, so a collection can run at the
// top of any of them. Whatever the data you pass points into must stay rooted
// on the value stack across the call: when the new object is derived from an
// existing one, build first and pop the consumed arguments after
// (vm_pop_prev_n). Popping first is only safe when the source is plain C
// memory the GC cannot free.
void vm_maybe_collect(VM *vm);
void vm_build_integer(VM *vm, Integer value);
void vm_build_bool(VM *vm, bool value);
void vm_build_float(VM *vm, double value);
void vm_build_builtin(VM *vm, BuiltinObject value);
void vm_build_nil(VM *vm);
void vm_build_lambda(VM *vm, bool is_variadic, Object *expr, Scope *scope);
void vm_build_symbol(VM *vm, StringName value);
void vm_build_string(VM *vm, const char *data, size_t size);
void vm_build_string_own(VM *vm, char *data, size_t size);
void vm_build_list(VM *vm);

// vm_stack.c
void vm_push(VM *vm, Object *value);
void vm_push_prev(VM *vm, Object *value);
Object *vm_peek(VM *vm);
Object *vm_prev(VM *vm);
Object *vm_peek_at(VM *vm, size_t n);
void vm_dup(VM *vm);
void vm_dup_prev(VM *vm);
void vm_swap(VM *vm);
void vm_rot(VM *vm);
void vm_pop(VM *vm);
void vm_pop_n(VM *vm, size_t n);
void vm_pop_prev(VM *vm);
void vm_pop_prev_n(VM *vm, size_t n);

// vm_ops.c
void vm_pack_list(VM *vm, size_t length);
size_t vm_unpack_list(VM *vm);

// vm_eval.c
void vm_eval_node(VM *vm);
// List (args, evaluated), Callable -> Node (result). Invokes the callable
// without evaluating the arguments again - the primitive for calling a
// function with values you already hold (map, filter, future apply).
void vm_call(VM *vm);
bool vm_cast_to_bool(VM *vm);
ObjectKind vm_to_common_numeric(VM *vm);

#endif
