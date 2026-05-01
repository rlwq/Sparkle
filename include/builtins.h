#ifndef BUILTINS_H
#define BUILTINS_H

#include "vm.h"

typedef struct {
    const char *name;
    LispBuiltin func;
} BuiltinDef;

extern const BuiltinDef BUILTINS[];
extern const size_t BUILTINS_COUNT;

void rkl_int_eq(VM *vm, size_t args_count);
void rkl_sub(VM *vm, size_t args_count);
void rkl_print(VM *vm, size_t args_count);
void rkl_cons(VM *vm, size_t args_count);
void rkl_car(VM *vm, size_t args_count);
void rkl_cdr(VM *vm, size_t args_count);
void rkl_add(VM *vm, size_t args_count);
void rkl_gt(VM *vm, size_t args_count);
void rkl_is_nil(VM *vm, size_t args_count);
void rkl_eval(VM *vm, size_t args_count);

#endif
