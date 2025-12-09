#ifndef UMN_UTILS_H
#define UMN_UTILS_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

/* Utilities for umn */

#define UMN_TODO(msg)                                        \
    do                                                       \
    {                                                        \
        printf("%s%d: TODO(%s)\n", __FILE__, __LINE__, msg); \
        exit(47);                                            \
    } while (0)

/* The fundamental data structure is what i'm calling a slice */
#define UMN_SLICE_T(Type)       \
    struct                      \
    {                           \
        size_t count, capacity; \
        Type *items;            \
    }

/* NOTE: must check items seperately */
#define umn_slice_reserve(slice, __amount)                                                     \
    do                                                                                         \
    {                                                                                          \
        if (((slice).count + __amount) >= (slice).capacity)                                    \
        {                                                                                      \
            (slice).capacity = ((slice).capacity + __amount + 1) * 2;                          \
            (slice).items = realloc((slice).items, (slice).capacity * sizeof(*(slice).items)); \
        }                                                                                      \
    } while (0)

/* returns the value pushed */
#define umn_slice_push(slice, v) \
    (slice).items[assert((slice).count < (slice).capacity), (slice).count++] = v
/* returns the value removed */
#define umn_slice_pop(slice) \
    (slice).items[assert((slice).count > 0), --(slice).count]
/* returns the value at the index */
#define umn_slice_at(slice, idx) \
    (slice).items[(slice).count * ((idx) < 0) + (idx)]

/* Experiment with the slab allocator thingy ... */
/* An implementation of a slab allocator which is just essentially a slice of slices ... */
#define UMN_SLAB_T(Type)           \
    struct                         \
    {                              \
        size_t count, capacity;    \
        UMN_SLICE_T(Type) * items; \
    }

#define UMN_SLAB_SIZE 128 /* just for the vibes just allocate 128 items because why not */

typedef UMN_SLAB_T(void) umn_Slab_Generic;
void *umn_slab_alloc_generic(umn_Slab_Generic *slab, size_t item_size);
#define umn_slab_alloc(slab) umn_slab_alloc_generic((umn_Slab_Generic *)&slab, sizeof((*(*slab.items).items)))

inline void *umn_slab_alloc_generic(umn_Slab_Generic *slab, size_t item_size)
{
    umn_slice_reserve((*slab), 1); /* assumes malloc zero initializes the data ... */
    assert(slab->items != NULL);

    UMN_SLICE_T(void) *slice = (void *)&umn_slice_at((*slab), -1);

    if (slice->capacity == 0)
    { /* initialize a new slab ... */
        umn_slice_at((*slab), -1).capacity = UMN_SLAB_SIZE * ((slab->count + 1));
        umn_slice_at((*slab), -1).items = malloc(umn_slice_at((*slab), -1).capacity * item_size);
        assert(umn_slice_at((*slab), -1).items != NULL);
        memset(umn_slice_at((*slab), -1).items, 0, umn_slice_at((*slab), -1).capacity * item_size);
    }

    if (umn_slice_at((*slab), -1).count < umn_slice_at((*slab), -1).capacity)
    {
        umn_slice_at((*slab), -1).count++;
        return (void *)((char *)umn_slice_at((*slab), -1).items + item_size * umn_slice_at((*slab), -1).count);
    }

    slab->count += 1;
    return umn_slab_alloc_generic(slab, item_size);
}

#endif