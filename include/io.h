#ifndef IO_H
#define IO_H

#include "dynamic_array.h"
#include "object.h"

typedef DA(char) CharDA;

// The single source of truth for an object's textual form: print and the str
// builtin must agree byte-for-byte, so both funnel through here.
void write_expr(CharDA *out, Object *expr);
void print_list(Object *expr);
void print_expr(Object *expr);

#endif
