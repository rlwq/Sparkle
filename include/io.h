#ifndef IO_H
#define IO_H

#include "dynamic_array.h"
#include "object.h"
#include "vm.h"

typedef DA(char) CharDA;

// The single source of truth for an object's textual form: print and the str
// builtin must agree byte-for-byte, so both funnel through here.
//
// Takes the VM for the sake of one comparison: a (quote x) list prints as 'x,
// and the head is recognised by StringName identity against VM_SYM(vm, quote)
// like every other reserved name.
void write_expr(VM *vm, CharDA *out, Object *expr);

#endif
