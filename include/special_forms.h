#ifndef SPECIAL_FORMS_H
#define SPECIAL_FORMS_H

#include "string_interner.h"
#include "vm.h"

typedef void (*SpecialFormHandler)(VM *vm);

typedef struct {
    const char *keyword;
    SpecialFormHandler func;
} SpecialFormDef;

extern const SpecialFormDef SPECIAL_FORMS[];
extern const size_t SPECIAL_FORMS_COUNT;

// Registers every entry of SPECIAL_FORMS with the VM. The table is read, never
// written: the interned keywords land in the VM, so the same table can be
// registered with any number of VMs. Call once per VM, before vm_run.
void special_forms_attach(VM *vm);

#endif
