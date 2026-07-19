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

// Installs special-form dispatch into the VM: interns every keyword in
// SPECIAL_FORMS in place (so dispatch compares interned StringNames by
// pointer) and sets vm->try_special. Call once per VM, before vm_run.
void special_forms_attach(VM *vm);

bool try_dispatch_special_form(VM *vm, StringName name);

#endif
