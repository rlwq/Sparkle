#ifndef VM_H
#define VM_H

#include "forwards.h"
#include "object.h"
#include "string_interner.h"

#include <setjmp.h>

#define VM_CURR_SCOPE(e_) (da_at_end((e_)->scope_stack, 0))

#define ASSERT_KIND(e_, k_)                                                                        \
    (assert(((e_)->value_stack.size > 0) && (da_at_end((e_)->value_stack, 0)->kind == (k_))))

// Wrapped in do/while so it is a single statement: a bare `if` here would let a
// trailing `else` at the call site bind to the macro's `if` instead.
#define VM_RECOVER_IF(vm_, expr_, ex_)                                                             \
    do {                                                                                           \
        if ((expr_))                                                                               \
            vm_recover((vm_), (ex_));                                                              \
    } while (0)

// Singletons are allocated once in vm_alloc and never again: vm_build_bool and
// vm_build_nil push these same objects, so comparing pointers against a
// singleton is an identity check.
#define X_RUNTIME_SINGLETONS                                                                       \
    X(Nil, KIND_NIL, {0})                                                                          \
    X(True, KIND_BOOL, {.bool_ = true})                                                            \
    X(False, KIND_BOOL, {.bool_ = false})

// The names the language itself gives meaning to: the symbols that evaluate to
// a value of their own (Nil, True, False), the head the reader expands 'x into,
// and the markers that fix argument positions in lambda and for.
//
// Held as Symbol objects allocated once in vm_alloc, so a dispatch test is a
// StringName pointer comparison - see VM_SYM. They live here rather than in the
// interner because the interner is a utility: which strings a language calls
// keywords is not its business.
//
// This is not their final home either: naming them here still has the VM core
// spelling out words only the language gives meaning to. They belong in lang/,
// registered into the VM the way special forms now are (see
// vm_register_special_form) - the mechanism already exists, so what is left is
// moving the list and the self-evaluating-symbol rule in vm_eval_symbol along
// with it.
#define X_LANGUAGE_SYMBOLS                                                                         \
    X(Nil)                                                                                         \
    X(True)                                                                                        \
    X(False)                                                                                       \
    X(quote)                                                                                       \
    X(Var)
// The interned name of a language symbol, for comparing against a StringName.
#define VM_SYM(vm_, name_) (OBJ_SYMBOL((vm_)->symbols._##name_))

#define X_RUNTIME_EXCEPTIONS                                                                       \
    X(TYPE_EXCEPTION, "Function expected some other object type.")                                 \
    X(ARITY_EXCEPTION, "Wrong arity for a function call.")                                         \
    X(UNDEFINED_EXCEPTION, "Symbol has no definition.")                                            \
    X(REBINDING_EXCEPTION, "Symbol is already bound.")                                             \
    X(UNCALLABLE_EXCEPTION, "Can't use this kind of object as a function.")                        \
    X(VALUE_EXCEPTION, "Function expected some other value.")

// A keyword bound to the handler that runs it. The keyword is interned at
// registration, so a lookup is a StringName pointer comparison.
typedef struct {
    StringName keyword;
    void (*func)(VM *vm);
} SpecialFormEntry;

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

    // Special forms installed by the language layer through
    // vm_register_special_form, the same way builtins arrive through
    // vm_register_builtin: the VM core holds the pairs and looks them up, but
    // names none of them. Empty means no forms installed.
    DA(SpecialFormEntry) special_forms;

    // Handed the value of every top-level expression, just before vm_run drops
    // it. NULL discards results, which is what running a file wants; a REPL
    // installs a sink that prints. The value is on the value stack at the call,
    // so it is rooted and the sink may allocate.
    //
    // A function pointer rather than a bool because rendering a value lives in
    // lang/: a bool would have vm_run reaching for write_expr, and src/vm/ has
    // been kept clear of the language layer on purpose.
    void (*on_result)(VM *vm, Object *result);

    struct {
#define X(name_, kind_, init_) Object *_##name_;
        X_RUNTIME_SINGLETONS
#undef X

#define X(name_, msg_) Object *_##name_;
        X_RUNTIME_EXCEPTIONS
#undef X
    } singletons;

    // Kept apart from singletons: the Symbol `Nil` and the Nil value are two
    // different objects, and both are needed.
    struct {
#define X(name_) Object *_##name_;
        X_LANGUAGE_SYMBOLS
#undef X
    } symbols;

    Object *exception;
    bool is_err;
};

// vm.c
VM *vm_alloc(GC *gc, StringInterner *si);

// Copies the pointers, so the caller may free its array right after. May be
// called repeatedly: whatever the scope stack reaches survives a cycle.
void vm_load_instructions(VM *vm, ObjectPtrDA instructions);

void vm_free(VM *vm);
void vm_mark(VM *vm);
void vm_register_builtin(VM *vm, const char *name, BuiltinObject func_ptr);

// Interns keyword and stores it with its handler. The caller keeps its own
// table constant: nothing here writes back into it.
void vm_register_special_form(VM *vm, const char *keyword, void (*func)(VM *vm));

// Runs the form registered for name and reports whether there was one. The
// evaluator consults this before treating a list head as a function call.
bool vm_try_special_form(VM *vm, StringName name);

// Clears is_err and exception on entry: they describe one run, not the VM.
void vm_run(VM *vm);

// vm_recovery.c
void vm_push_recovery(VM *vm, jmp_buf *jmp);
void vm_pop_recovery(VM *vm);
_Noreturn void vm_recover(VM *vm, Object *exception_symbol) __attribute__((cold));
void vm_expect(VM *vm, ObjectType type);
void vm_expect2(VM *vm, ObjectType prev, ObjectType peek);

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
void vm_eval_object(VM *vm);
void vm_call(VM *vm);
bool vm_cast_to_bool(VM *vm);
ObjectKind vm_to_common_numeric(VM *vm);

#endif
