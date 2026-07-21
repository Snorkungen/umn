#ifndef UMN_UTILS_H
#define UMN_UTILS_H

#define UMN_CORE_IMPLEMENTATION
#include "./core.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

/* Utilities for umn */



#define umn_assert(expr) ((expr) ? (void)0 : umn_panic("assert failed", __FILE__, __LINE__))

#define UMN_TODO(msg)                                     \
    do                                                    \
    {                                                     \
        umn_panic("TODO(" msg ")\n", __FILE__, __LINE__); \
    } while (0) //  printf("%s:%d: TODO(%s)\n", __FILE__, __LINE__, msg);

/* for now this is just a placeholder */
typedef struct umn_allocator_t
{
    struct umn_allocator_t *allocator;
} umn_allocator_t;

void *umn_malloc(umn_allocator_t *allocator, size_t size);
void *umn_realloc(umn_allocator_t *allocator, void *allocation, size_t size);
void umn_free(umn_allocator_t *allocator, void *allocation);

/* The fundamental data structure is what i'm calling a slice */
#define UMN_SLICE_T(Type)           \
    struct                          \
    {                               \
        umn_allocator_t *allocator; \
        size_t count, capacity;     \
        Type *items;                \
    }

#define umn_slice_reserve(slice, __amount)                                                                          \
    do                                                                                                              \
    {                                                                                                               \
        if (((slice).count + (__amount)) >= (slice).capacity)                                                       \
        {                                                                                                           \
            (slice).capacity = ((slice).capacity + ((slice).count + (__amount)) - (slice).capacity) * 2;            \
            (slice).capacity += (slice).capacity % 8;                                                               \
            (slice).items = umn_realloc(slice.allocator, (slice).items, (slice).capacity * sizeof(*(slice).items)); \
        }                                                                                                           \
    } while (0)

#define umn_slice_ensure(slice) umn_slice_reserve(slice, 0)

/* returns the value pushed */
#define umn_slice_push(slice, v) \
    ((slice).items[umn_assert((slice).count < (slice).capacity), (slice).count++] = (v))
/* returns the value removed */
#define umn_slice_pop(slice) \
    (slice).items[umn_assert((slice).count > 0), --(slice).count]
/* returns the value at the index */
#define umn_slice_at(slice, idx) \
    (slice).items[(slice).count * ((idx) < 0) + (idx)]
#define umn_slice_last(slice) \
    (slice).items[(slice).count - 1]

/* Experiment with the slab allocator thingy ... */
/* An implementation of a slab allocator which is just essentially a slice of slices ... */
/* TODO: this should be an allocator */
#define UMN_SLAB_T(Type)            \
    struct                          \
    {                               \
        umn_allocator_t *allocator; \
        size_t count, capacity;     \
        struct                      \
        {                           \
            size_t count, capacity; \
            Type *items;            \
        } *items;                   \
    }

#define UMN_SLAB_SIZE 128 /* just for the vibes just allocate 128 items because why not */

typedef UMN_SLAB_T(void) umn_slab_generic_t;
void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size);
#define umn_slab_alloc(slab) umn_slab_alloc_generic((umn_slab_generic_t *)&(slab), sizeof((*(*(slab).items).items)))

void umn_slab_drop_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);
void umn_slab_free_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);
#define umn_slab_free(slab, allocated) umn_slab_free_generic((umn_slab_generic_t *)&(slab), allocated, sizeof((*(*(slab).items).items)))
#define umn_slab_drop(slab, allocated) umn_slab_drop_generic((umn_slab_generic_t *)&(slab), allocated, sizeof((*(*(slab).items).items)))

void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size);
void umn_slab_drop_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);

unsigned long umn_readu(const char *s, size_t n, int base); /* decode string to number */
double umn_readd(const char *s, size_t n);                  /* decode string to double */

typedef UMN_SLAB_T(char) umn_arena_t;
void *umn_arena_alloc(umn_arena_t *arena, size_t size);
void *umn_arena_alloc(umn_arena_t *arena, size_t size);

#ifdef UMN_UTILS_IMPLEMENTATION /* UMN_UTILS_IMPLEMENTATION */
#undef UMN_UTILS_IMPLEMENTATION

void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size)
{
    /* handle base case */
    if (slab->count == 0 || umn_slice_at((*slab), -1).count == umn_slice_at((*slab), -1).capacity)
    {
        slab->count += 1;

        size_t begin_cap = slab->capacity;
        umn_slice_reserve((*slab), 0);
        /* initialize the items */
        umn_assert((*slab).items);
        umn_memset(slab->items + begin_cap, 0, sizeof(*slab->items) * (slab->capacity - begin_cap));

        if (slab->items == NULL)
            return NULL;

        /* allocate the block for the slot */
        if (umn_slice_at(*slab, -1).capacity == 0)
        {
            umn_slice_at((*slab), -1).count = 0;
            umn_slice_at((*slab), -1).capacity = UMN_SLAB_SIZE;
            umn_slice_at((*slab), -1).items = umn_malloc(slab->allocator, umn_slice_at((*slab), -1).capacity * item_size);
        }

        if (umn_slice_at((*slab), -1).items == NULL)
            return NULL;
    }

    umn_slice_at((*slab), -1).count++;

    void *alllocation = (&((char *)umn_slice_at((*slab), -1).items)[(umn_slice_at((*slab), -1).count - 1) * item_size]);
    umn_memset(alllocation, 0, item_size);

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
    for (i = slab->count; (size_t)i < slab->capacity; i++)
        umn_slice_at(*slab, i).count = 0;
}

/* free the node and the nodes allocated after */
void umn_slab_free_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size)
{
    umn_slab_drop_generic(slab, allocated, item_size);

    for (unsigned i = slab->count; i < slab->capacity; i++)
        umn_free(slab->allocator, slab->items[i].items);

    umn_memset(slab->items + slab->count, 0, sizeof(*slab->items) * (slab->capacity - slab->count));

    if (slab->count == 0)
    {
        umn_free(slab->allocator, slab->items);
        umn_memset(slab, 0, sizeof(*slab));
    }
}

/* --- UMN SB ---*/

unsigned long umn_readu(const char *s, size_t n, int base)
{
    const char *end = s + n;
    unsigned long result = 0, tmp;

    if (n > 2)
    {
        if ((*(s + 1) | 32) == 'b')
            base = 2, s += 2;
        else if ((*(s + 1) | 32) == 'x')
            base = 16, s += 2;
    }
    else if (n > 1 && *s == '0' && umn_isodigit(*(s + 1)))
        base = 8, s += 1;

    switch ((unsigned)base & 0xFF)
    {
    case 2:
        for (; s < end; s++)
            result = (result << 1) | ((unsigned)*s - '0');
        break;
    case 8:
        for (; s < end; s++)
            result = (result << 3) | ((unsigned)*s - '0');
        break;
    case 16:
        for (; s < end; s++)
        {
            tmp = ((unsigned)*s - '0'), tmp = tmp <= 9 ? tmp : 10 + (((unsigned)*s | 32) - 'a');
            result = (result << 4) | tmp;
        }
        break;
    case 10:
    default:
        for (; s < end; s++)
            result = (result * 10) + ((unsigned)*s - '0');
    }

    return result;
}

double umn_readd(const char *s, size_t n)
{
    char *endptr = (char *)s + n;
    return umn_strtod(s, &endptr);
}
/* the arena is a special form of the slab allocator .... */
void *umn_arena_alloc(umn_arena_t *arena, size_t size)
{
    static const size_t slab_size = 1024 * 4; /* 4kiB should be enough */
    void *data;

    /* TODO: align the thing ...  */
    if (arena->count == 0 || (umn_slice_at((*arena), -1).count + size) >= umn_slice_at((*arena), -1).capacity)
    {
        umn_slice_reserve((*arena), 1);
        arena->count++;

        umn_slice_at((*arena), -1).capacity = slab_size > size ? slab_size : size;
        umn_slice_at((*arena), -1).items = umn_malloc(arena->allocator, umn_slice_at((*arena), -1).capacity);
    }

    data = umn_slice_at((*arena), -1).items + umn_slice_at((*arena), -1).count;
    umn_slice_at((*arena), -1).count += size;
    return data;
}

void umn_arena_free(umn_arena_t *arena)
{
    umn_slab_free((*arena), NULL);
}

void *umn_malloc(umn_allocator_t *allocator, size_t size)
{
    (void)allocator;
    return NULL;
}

void *umn_realloc(umn_allocator_t *allocator, void *allocation, size_t size)
{
    (void)allocator;
    return NULL;
}

void umn_free(umn_allocator_t *allocator, void *allocation)
{
    (void)allocator, (void)allocation;
    NULL;
}

#endif
#endif
