#ifndef SPECIAL_FORMS_H
#define SPECIAL_FORMS_H

#include "string_interner.h"
#include "vm.h"

typedef void (*SpecialFormHandler)(VM *vm);

typedef struct {
    const char *keyword;
    SpecialFormHandler func;
} SpecialFormDef;

extern SpecialFormDef SPECIAL_FORMS[];
extern size_t SPECIAL_FORMS_COUNT;

// Interns every keyword in SPECIAL_FORMS in place, so dispatch can compare
// interned StringNames by pointer. Call once, before vm_run.
void special_forms_init(StringInterner *si);

bool try_dispatch_special_form(VM *vm, StringName name);

#endif
