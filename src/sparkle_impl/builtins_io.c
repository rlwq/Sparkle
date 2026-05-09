#include "builtins.h"
#include "io.h"

#include <stdio.h>

void rkl_print(VM *vm) {
    LispNode *list = vm_peek(vm);
    while (list->kind == LISP_CONS) {
        print_expr(CAR(list));
        printf("\n");
        list = CDR(list);
    }
    vm_pop(vm);
    vm_build_nil(vm);
}

DEFINE_MODULE(IO) = {{"print", {rkl_print, 0, true}}};
DEFINE_MODULE_SIZE(IO);
