#include "builtins.h"
#include "object.h"
#include "vm.h"

#define rkl_type_predicate(type_)                                                                  \
    void rkl_is_##type_(VM *vm) {                                                                  \
        bool result = OFTYPE(vm_peek(vm), TY_##type_);                                             \
        vm_pop(vm);                                                                                \
        vm_build_bool(vm, result);                                                                 \
    }

#define X(t_) rkl_type_predicate(t_)
X_KINDS
#undef X

DEFINE_MODULE(TYPE_PREDICATES) = {
#define X(t_) {"?" #t_, rkl_is_##t_, 1, false},
    X_KINDS};

DEFINE_MODULE_SIZE(TYPE_PREDICATES);
