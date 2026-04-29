#ifndef FORWARDS_H
#define FORWARDS_H

#include "dynamic_array.h"

typedef struct VM VM;
typedef struct GC GC;
typedef struct LispNode LispNode;
typedef DA(struct LispNode *) LispNodePtrDA;
typedef struct Scope Scope;

#endif
