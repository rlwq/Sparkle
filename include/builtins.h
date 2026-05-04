#ifndef BUILTINS_H
#define BUILTINS_H

#include "vm.h"

typedef struct {
    const char *name;
    LispBuiltin func;
} BuiltinDef;

extern const BuiltinDef BUILTINS[];
extern const size_t BUILTINS_COUNT;

void rkl_int_eq(VM *vm);
void rkl_sub(VM *vm);
void rkl_print(VM *vm);
void rkl_cons(VM *vm);
void rkl_car(VM *vm);
void rkl_cdr(VM *vm);
void rkl_add(VM *vm);
void rkl_gt(VM *vm);
void rkl_is_nil(VM *vm);
void rkl_eval(VM *vm);

#endif
