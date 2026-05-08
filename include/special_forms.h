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

#endif
