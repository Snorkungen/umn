#ifndef UMN_UTILS_H
#define UMN_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

/* Utilities for umn */

#define UMN_TODO(msg)                                         \
    do                                                        \
    {                                                         \
        printf("%s:%d: TODO(%s)\n", __FILE__, __LINE__, msg); \
        abort();                                              \
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
        if (((slice).count + (__amount)) >= (slice).capacity)                                  \
        {                                                                                      \
            (slice).capacity = ((slice).capacity + (__amount) + 1) * 2;                        \
            (slice).items = realloc((slice).items, (slice).capacity * sizeof(*(slice).items)); \
        }                                                                                      \
    } while (0)

/* returns the value pushed */
#define umn_slice_push(slice, v) \
    ((slice).items[assert((slice).count < (slice).capacity), (slice).count++] = v)
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

typedef UMN_SLAB_T(void) umn_slab_generic_t;
void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size);
#define umn_slab_alloc(slab) umn_slab_alloc_generic((umn_slab_generic_t *)&(slab), sizeof((*(*(slab).items).items)))

void umn_slab_drop_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);
void umn_slab_free_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);
#define umn_slab_free(slab, allocated) umn_slab_free_generic((umn_slab_generic_t *)&(slab), allocated, sizeof((*(*(slab).items).items)))
#define umn_slab_drop(slab, allocated) umn_slab_drop_generic((umn_slab_generic_t *)&(slab), allocated, sizeof((*(*(slab).items).items)))

void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size)
{
    /* handle base case */
    if (slab->count == 0 || umn_slice_at((*slab), -1).count == umn_slice_at((*slab), -1).capacity)
    {
        slab->count += 1;

        size_t begin_cap = slab->capacity;
        umn_slice_reserve((*slab), 0);
        /* initialize the items */
        memset(slab->items + begin_cap, 0, sizeof(*slab->items) * (slab->capacity - begin_cap));

        if (slab->items == NULL)
            return NULL;

        /* allocate the block for the slot */
        if (umn_slice_at(*slab, -1).capacity == 0)
        {
            umn_slice_at((*slab), -1).count = 0;
            umn_slice_at((*slab), -1).capacity = UMN_SLAB_SIZE;
            umn_slice_at((*slab), -1).items = malloc(umn_slice_at((*slab), -1).capacity * item_size);
        }

        if (umn_slice_at((*slab), -1).items == NULL)
            return NULL;
    }

    umn_slice_at((*slab), -1).count++;

    void *alllocation = (&((char *)umn_slice_at((*slab), -1).items)[(umn_slice_at((*slab), -1).count - 1) * item_size]);
    memset(alllocation, 0, item_size);

    return alllocation;
}

/* freeing every time is wasteful */
void umn_slab_drop_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size)
{
    int i, j;

    /* determine the slab coords for the thing ... */
    for (i = -1 + slab->count; i >= 0; --i)
    {
        if (allocated >= umn_slice_at(*slab, i).items && ((size_t)allocated - (size_t)umn_slice_at(*slab, i).items) < (size_t)umn_slice_at(*slab, i).count * item_size)
        {
            j = ((size_t)allocated - (size_t)umn_slice_at(*slab, i).items) / item_size;
            break;
        }
    }

    if (i < 0)
        return;

    umn_slice_at(*slab, i).count = j;
    if (umn_slice_at(*slab, i).count == 0)
        i--;

    slab->count = i + 1;
    for (i = slab->count; i < slab->capacity; i++)
        umn_slice_at(*slab, i).count = 0;
}

/* free the node and the nodes allocated after */
void umn_slab_free_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size)
{
    umn_slab_drop_generic(slab, allocated, item_size);

    for (unsigned i = slab->count; i < slab->capacity; i++)
        free(slab->items[i].items);

    memset(slab->items + slab->count, 0, sizeof(*slab->items) * (slab->capacity - slab->count));

    if (slab->count == 0)
    {
        free(slab->items);
        memset(slab, 0, sizeof(*slab));
    }
}

#endif
