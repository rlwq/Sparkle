#include "builtins.h"
#include "object.h"
#include "vm.h"

// Value -> Bool
#define spk_type_predicate(type_)                                                                  \
    static void spk_is_##type_(VM *vm) {                                                           \
        bool result = OBJ_OFTYPE(vm_peek(vm), TY_##type_);                                         \
        vm_pop(vm);                                                                                \
        vm_build_bool(vm, result);                                                                 \
    }

#define X(t_) spk_type_predicate(t_)
X_KINDS
#undef X

DEFINE_MODULE(TYPE_PREDICATES) = {
#define X(t_) {"?" #t_, spk_is_##t_, 1, false},
    X_KINDS};
#undef X

DEFINE_MODULE_SIZE(TYPE_PREDICATES);
