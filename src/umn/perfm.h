#ifndef UMN_PERFM_H
#define UMN_PERFM_H

#include "core.h"

#ifdef __x86_64__
#include <x86intrin.h>
#define umn_perfm_tsc() (_rdtsc())
#endif

typedef struct umn_perfm_t
{
    struct
    {
        const char *name;
        struct umn_perfm_t *parent, *child, *sibling;
        char name_buffer[8];
    } data;

    struct
    {
        unsigned long long v, n, start;
    } avg;
} umn_perfm_t;

static struct
{
    size_t count, capacity;
    struct
    {
        size_t count, capacity;
        umn_perfm_t *items;
    } *items;

    /* here i can shove other things in the future */
} umn_perfm_ctx = {0};

#define umn_perfm_reset(t) umn_memset(&t->avg, 0, sizeof(t->avg))

umn_perfm_t *umn_perfm_create(void)
{
    umn_perfm_t *t = umn_slab_alloc(umn_perfm_ctx);
    umn_assert(t);
    umn_perfm_reset(t);
    return t;
}
umn_perfm_t *umn_perfm_create_new(umn_perfm_t *parent, const char *name)
{
    umn_perfm_t *t = umn_slab_alloc(umn_perfm_ctx);
    umn_assert(t);
    umn_memset(t, 0, sizeof(*t));

    if (name)
        t->data.name = name;
    else /* auto generate a \consistent\ name */
    {
        t->data.name = t->data.name_buffer;
        t->data.name_buffer[0] = '@';
        t->data.name_buffer[1] = '1';
    }

    if (parent)
    {
        t->data.parent = parent;
        umn_perfm_t *sibling = t->data.parent->data.child;

        while (sibling && sibling->data.sibling)
            sibling = sibling->data.sibling;

        if (parent->data.child == NULL)
            parent->data.child = t;
        else
            sibling->data.sibling = t;
    }

    return t;
}

void umn_perfm_setup(umn_perfm_t *t, const char *name, umn_perfm_t *parent)
{
    t->data.name = name;
    t->data.name = name;
    t->data.parent = parent;
    if (parent && parent->data.child)
    {
        t->data.sibling = parent->data.child;
        parent->data.child = t;
        /* let's just do it in constant time because it's easier */
    }
    else if (parent)
        parent->data.child = t;

    if (name == NULL)
    {
        /* */
    }
}

static unsigned umn_perfm_open(umn_perfm_t *t);
static inline unsigned umn_perfm_open(umn_perfm_t *t)
{
    t->avg.start = umn_perfm_tsc();
    return 1;
}

static unsigned umn_perfm_stop(umn_perfm_t *t);
static inline unsigned umn_perfm_stop(umn_perfm_t *t)
{
    /* avg += ((t2 - t1) - avg) / (n + 1)*/
    t->avg.v = (long long)t->avg.v +
               ((long long)(umn_perfm_tsc() - t->avg.start) - (long long)t->avg.v) /
                   (long long)(++t->avg.n);

    t->avg.start = (size_t)-1; /* just incase so that this will hopefully create an obvious errror */
    return 0;
}

#define umn_perfm_open_for(t) for (               \
    int __umn_perfm_toggle__ = umn_perfm_open(t); \
    __umn_perfm_toggle__;                         \
    __umn_perfm_toggle__ = umn_perfm_stop(t))

void umn_perfm_report(umn_perfm_t *t)
{
    char buffer[128] = {0}, *dest = buffer;
    const char *name = t->data.name;

    unsigned long long avg = t->avg.v;

    umn_printf("%s = %llu", name, avg);

    // iterate over children

    /* account for the  overhead of the perfm thing */

    for (umn_perfm_t *child = t->data.child; child; child = child->data.sibling)
    {
        umn_printf(", .%s = %llu %.0Lf%%",
               child->data.name,
               child->avg.v, (long double)child->avg.v / t->avg.v * 100);

        umn_perfm_reset(child);
    }

    umn_perfm_reset(t);

    umn_printc('\n');
}

/*
id = umn_perfm_create()

umn_perfm_open(id)
umn_perfm_stop(id)

umn_perfm_get(id).avg

V2 more user friendly

umn_perfm_create(NULL, "name")

umn_perfm_open_for("name")
    ... statement


*/

#endif
