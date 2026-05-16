#include "builtins.h"
#include "io.h"
#include "object.h"
#include "vm.h"
#include <stdio.h>

void rkl_print(VM *vm) {
    LIST_ITER(vm, list, vm_peek(vm))
        print_expr(CAR(list));
        printf("\n");
        END_LIST_ITER_RECOVER(vm, list)

        vm_pop(vm);
        vm_build_nil(vm);
    }
    DEFINE_MODULE(IO) = {{"print", rkl_print, 0, true}};
    DEFINE_MODULE_SIZE(IO);
