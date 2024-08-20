/*
    UMN-APA -- parse arguements in form
    > key1 key2 --flag1 data-for-flag1 data-for-flag1 --flag2 --flag3 data-for-flag3
*/

#ifndef UMN_APA_H
#define UMN_APA_H

#include <stddef.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "umn-arena.h"

struct UMN_APA_Flag_Definition
{
    char *name;      /* the textual representation of the flag ... */
    char short_name; /* a character that the flag could be referred with ... */

    /* could have some kind of meta-data maybe idk */
    char contiguous; /* if true will try to combine values into a space-seperated string */
};

/* this is a header */
struct UMN_APA_Flag_Values
{
    int visitor_count;

    size_t capacity;
    size_t count;
    char **values;
};

struct UMN_APA_Result
{
    int argc;
    char **argv;

    int flagc;
    struct UMN_APA_Flag_Definition *flagd;
    struct UMN_APA_Flag_Values *flags;

    int keyc;    /* the number of keys */
    char **keys; /* keys entered */
};

/* returns truthy value if value mets criteria for being a flag ... */
int umn_apa_parse_is_flag(char *s)
{
    if (s[0] != '-' || strlen(s) < 2 || strcmp(s, "--") == 0)
    {
        return 0;
    }

    /* flags do not begin with numeric values */
    char firstc = s[1] == '-' ? s[2] : s[1];
    if (isdigit(firstc))
    {
        return 0;
    }

    return 1;
}

/* error returns non-zero value, result object gives the arguemnts and flag definitions */
int umn_apa_parse(struct UMN_Arena *arena, struct UMN_APA_Result *result)
{
    if (arena == NULL || result == NULL)
    {
        return -1;
    }

    if (result->argv == NULL)
    {
        return -1;
    }

    /* INITIALIZE RESULT VALUES ... */
    result->keyc = 0;
    result->keys = umn_arena_alloc(arena, sizeof(char *) * result->argc);

    result->flags = umn_arena_alloc(arena, sizeof(struct UMN_APA_Flag_Values) * (result->flagc + 1)); /* allocate a room to place unknown flags */
    memset(result->flags, 0, sizeof(struct UMN_APA_Flag_Values) * (result->flagc + 1));

    const size_t DEFAULT_FLAG_VALUES_CAP = 10;
    const size_t undefined_flag_index = result->flagc;

    size_t i = 0;

    /* read until first flag is found */
    for (; i < result->argc; i++)
    {
        assert(result->argv[i] != NULL);

        if (umn_apa_parse_is_flag(result->argv[i]))
        {
            break;
        }

        /* copy value into keys ... */
        /* assume that above logic allocated enough room for the keys */
        int key_len = strlen(result->argv[i]);
        result->keys[result->keyc] = umn_arena_alloc(arena, key_len + 1);
        strncpy(result->keys[result->keyc], result->argv[i], key_len);
        result->keys[result->keyc][key_len] = '\0'; /* ensure that the last byte is zero */

        result->keyc += 1;
    }

    /* start reading flags */
    /* store state of what flag is being read */

    size_t flag_idx = undefined_flag_index;

    for (; i < result->argc; i++)
    {
        assert(result->argv[i] != NULL);
        struct UMN_APA_Flag_Definition *fd;
        struct UMN_APA_Flag_Values *fv;

        if (umn_apa_parse_is_flag(result->argv[i]))
        {
            char *s = result->argv[i];
            /* match flag to flag defintion ... */
            for (flag_idx = 0; flag_idx < result->flagc; flag_idx++)
            {
                fd = result->flagd + flag_idx;

                if (fd->short_name == s[1] || strcmp(fd->name, s) == 0)
                {
                    break;
                }
            }

            assert(flag_idx <= undefined_flag_index);
        }

        /* set up facility for flag values ... */
        fd = result->flagd + flag_idx;
        fv = result->flags + flag_idx;

        fv->visitor_count++;

        size_t value_idx = fv->count;

        /* read the flag values */
        for (i++; i < result->argc; i++)
        {
            /* INITIALIZE FLAGS VALUE */
            assert(result->flags != NULL);
            if (fv->count >= fv->capacity)
            {
                if (fv->count == 0)
                {
                    /* allocate the current allocation ... */
                    fv->capacity = DEFAULT_FLAG_VALUES_CAP;
                    fv->values = umn_arena_alloc(arena, (sizeof(char *)) * fv->capacity);
                    assert(fv->values != NULL);
                }
                else
                {
                    /* resize the curren allocation ...*/
                    fv->capacity = fv->capacity * 2;
                    fv->values = umn_arena_realloc(arena, fv->values, (sizeof(char *)) * fv->capacity);
                    assert(fv->values != NULL);
                }
            }

            if (umn_apa_parse_is_flag(result->argv[i]))
            {
                break;
            }

            size_t arg_len = strlen(result->argv[i]);
            char *s;
            if (fd->contiguous && fv->values[value_idx] != NULL)
            {
                fv->count = value_idx;
                size_t prev_arg_len = strlen(fv->values[value_idx]);

                fv->values[value_idx] = s = umn_arena_realloc(arena, fv->values[value_idx], prev_arg_len + arg_len + 1);
                s += prev_arg_len;
                s[0] = ' ';
                s++;
            }
            else
            {
                fv->values[fv->count] = s = umn_arena_alloc(arena, arg_len + 1);
            }

            assert(s != NULL);
            /* copy arg to flag value */
            strncpy(s, result->argv[i], arg_len);
            s[arg_len] = '\0';
            fv->count++;
        }
        i--; /* compensate for the above loop */
    }

    return 0;
}

#endif