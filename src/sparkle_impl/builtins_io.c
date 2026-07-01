#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"
#include <stdio.h>

void rkl_print(VM *vm) {
    Object *args_ = vm_peek(vm);
    LIST_FOREACH(item, args_)
        print_expr(item);
        printf("\n");
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_nil(vm);
}

DEFINE_MODULE(IO) = {{"print", rkl_print, 0, true}};
DEFINE_MODULE_SIZE(IO);
