#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <assert.h>
#include <stdlib.h>

#define DA_INIT_CAPACITY 64
#define DA(T_) struct { T_ *data; size_t size; size_t capacity; }

#define da_init(da_)                                                      \
    do {                                                                  \
        (da_).capacity = DA_INIT_CAPACITY;                                \
        (da_).size = 0;                                                   \
        (da_).data = malloc((da_).capacity * sizeof(*(da_).data));        \
        assert((da_).data != NULL);                                       \
    } while (0)

#define da_push(da_, item_)                                                  \
    do {                                                                     \
        assert((da_).data);                                                  \
        if ((da_).size >= (da_).capacity) {                                  \
            (da_).capacity *= 2;                                             \
            (da_).data = realloc((da_).data,                                 \
                                 (da_).capacity * sizeof(*(da_).data));      \
            assert((da_).data != NULL);                                      \
        }                                                                    \
        (da_).data[(da_).size++] = (item_);                                  \
    } while (0)

#define da_pop(da_)          \
    do {                     \
        assert((da_).size);  \
        (da_).size--;        \
    } while (0)


#define da_at(da_, i_) ((da_).data[i_])
#define da_at_end(da_, i_) ((da_).data[(da_).size - i_ - 1])

#define da_nullify(da_)      \
    do {                     \
        (da_).data = NULL;   \
        (da_).size = 0;      \
        (da_).capacity = 0;  \
    } while (0)


#define da_free(da_)         \
    do {                     \
        free((da_).data);    \
        da_nullify(da_);     \
    } while (0)

#endif
