#ifndef SPECIAL_FORMS_H
#define SPECIAL_FORMS_H

#include "vm.h"

typedef void (*SpecialFormHandler)(VM *vm, size_t argc);

typedef struct {
    const char *keyword;
    SpecialFormHandler func;
} SpecialFormDef;

extern SpecialFormDef SPECIAL_FORMS[];
extern size_t SPECIAL_FORMS_COUNT;

bool try_dispatch_special_form(VM *vm);
void eval_let_form(VM *vm, size_t argc);
void eval_if_form(VM *vm, size_t argc);
void eval_lambda_form(VM *vm, size_t argc);
void eval_try_form(VM *vm, size_t argc);
void eval_quote_form(VM *vm, size_t argc);

#endif

