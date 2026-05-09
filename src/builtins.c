#include "builtins.h"
#include "vm.h"

#define REGISTER_MODULE(name_)                                                                     \
    for (size_t i = 0; i < BUILTINS_##name_##_COUNT; i++)                                          \
        vm_register_builtin(vm, BUILTINS_##name_[i].name, BUILTINS_##name_[i].func);

void register_builtins(VM *vm) {
#define X REGISTER_MODULE
    X_MODULES
#undef X
}
