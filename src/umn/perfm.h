#ifndef UMN_PERFM_H
#define UMN_PERFM_H

#include "utils.h"
#include <stdint.h>

#ifdef __x86_64__
#include <x86intrin.h>
#define umn_perfm_tsc() (_rdtsc())
#endif

typedef struct
{
    /* I think this can be unsigned */
    unsigned long long mavg; /* moving average */
    unsigned long long n;    /* count */
    unsigned long long time; /* the time of opening */
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

#define umn_perfm_reset(t) ((*t) = (umn_perfm_t){.time = (size_t)-1})

umn_perfm_t *umn_perfm_create(void)
{
    umn_perfm_t *t = umn_slab_alloc(umn_perfm_ctx);
    umn_perfm_reset(t);
    return t;
}

static unsigned umn_perfm_open(umn_perfm_t *t);
static inline unsigned umn_perfm_open(umn_perfm_t *t)
{
    t->time = umn_perfm_tsc();
    // printf("opened = %llu\n", t->time);
    return 1;
}

static unsigned umn_perfm_stop(umn_perfm_t *t);
static inline unsigned umn_perfm_stop(umn_perfm_t *t)
{
    /* avg += ((t2 - t1) - avg) / (n + 1)*/
    t->mavg = (long long)t->mavg + ((long long)(umn_perfm_tsc() - t->time) - (long long)(t->mavg)) / (long long)(++t->n);

    t->time = (size_t)-1; /* just incase so that this will hopefully create an obvious errror */
    return 0;
}

#define umn_perfm_open_stop(t) for (          \
    int __umn_perfm_toggle__ = umn_perfm_open(t); \
    __umn_perfm_toggle__;                     \
    __umn_perfm_toggle__ = umn_perfm_stop(t))

/*
id = umn_perfm_create()

umn_perfm_open(id)
umn_perfm_stop(id)

umn_perfm_get(id).avg
*/

#endif
