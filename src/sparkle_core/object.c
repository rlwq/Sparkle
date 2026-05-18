#include "object.h"
#include <assert.h>

bool is_proper_list(Object *object) {
    assert(OFTYPE(object, TY_LISTFUL));
    while (OFTYPE(object, TY_CONS))
        object = CDR(object);
    return OFTYPE(object, TY_NIL);
}
