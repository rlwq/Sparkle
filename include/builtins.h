#ifndef BUILTINS_H
#define BUILTINS_H

#include "vm.h"

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
    X(ARITHMETIC_LOGIC)                                                                            \
    X(CONS_LIST)                                                                                   \
    X(IO)

#define X DEFINE_MODULE_HEADER
X_MODULES
#undef X

void register_builtins(VM *vm);

#endif
