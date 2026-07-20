#ifndef BUILTINS_H
#define BUILTINS_H

#include "vm.h"

// A builtin is called with its arguments already evaluated, arity-checked and
// unpacked on the value stack; a variadic one additionally gets the trailing
// arguments packed in a list on top. It must consume them and leave exactly
// one result. Each implementation states its stack effect as a comment, top of
// the stack rightmost: `String, Integer -> String`. Do all vm_expect /
// VM_RECOVER_IF checks before allocating or building anything - vm_recover
// unwinds and never returns.
typedef struct {
    const char *name;
    void (*func)(VM *vm);
    size_t arity;
    bool is_variadic;
} BuiltinDef;

#define DEFINE_MODULE_HEADER(name_)                                                                \
    extern const BuiltinDef BUILTINS_##name_[];                                                    \
    extern const size_t BUILTINS_##name_##_COUNT;

#define DEFINE_MODULE_SIZE(name_)                                                                  \
    const size_t BUILTINS_##name_##_COUNT = sizeof(BUILTINS_##name_) / sizeof(BuiltinDef)

#define DEFINE_MODULE(name_) const BuiltinDef BUILTINS_##name_[]

#define X_MODULES                                                                                  \
    X(TYPE_PREDICATES)                                                                             \
    X(ARITHMETIC_LOGIC)                                                                            \
    X(LIST)                                                                                        \
    X(STRING)                                                                                      \
    X(IO)

#define X DEFINE_MODULE_HEADER
X_MODULES
#undef X

void register_builtins(VM *vm);

#endif
