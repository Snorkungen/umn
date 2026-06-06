#ifndef UMN_UTILS_H
#define UMN_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdio.h>  /* printf */
#include <stdlib.h> /* abort, free, malloc, realloc */
#include <string.h> /* memcpy, strlen, memset, strtoul */

#define umn_strlen(__str__) strlen(__str__)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

/* Utilities for umn */

#include <assert.h> /* assert */
#define UMN_ASSERT(expr) assert(expr)

/* umn_ctypes */
#define umn_isdigit(c) ((unsigned)(c) - '0' < 10)

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
#define umn_slice_ensure(slice)                                                                       \
    do                                                                                                \
    {                                                                                                 \
        if ((slice).count >= (slice).capacity)                                                        \
        {                                                                                             \
            (slice).capacity = ((((slice).capacity + (slice).count - (slice).capacity) * 2) + 7) / 8; \
            (slice).items = realloc((slice).items, (slice).capacity * sizeof(*(slice).items));        \
            UMN_ASSERT((slice).items);                                                                \
        }                                                                                             \
    } while (0)

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
    ((slice).items[UMN_ASSERT((slice).count < (slice).capacity), (slice).count++] = (v))
/* returns the value removed */
#define umn_slice_pop(slice) \
    (slice).items[UMN_ASSERT((slice).count > 0), --(slice).count]
/* returns the value at the index */
#define umn_slice_at(slice, idx) \
    (slice).items[(slice).count * ((idx) < 0) + (idx)]
#define umn_slice_last(slice) \
    (slice).items[(slice).count - 1]

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

void *umn_slab_alloc_generic(umn_slab_generic_t *slab, size_t item_size);
void umn_slab_drop_generic(umn_slab_generic_t *slab, void *allocated, size_t item_size);

/* String Builder */

typedef struct
{
    size_t kind;

    /* kind = 0*/
    const char *s;
    size_t length;

    /* kind = 1 */
    int width, precision;
    int modifier, conversion;
    char flag;
    bool width_set, precision_set;
} umn_format_command_t;

typedef struct
{
    size_t count, capacity;
    umn_format_command_t items[32];
} umn_format_commands_t;

typedef UMN_SLICE_T(char) umn_sb_t;
int umn_sb_pushc(umn_sb_t *sb, char c);                     /* push a single char */
int umn_sb_pushcc(umn_sb_t *sb, char c, size_t count);      /* push a given char n times */
int umn_sb_pushsn(umn_sb_t *sb, const char *s, size_t len); /* push a string up to n*/
int umn_sb_pushs(umn_sb_t *sb, const char *s);              /* push a string */
int umn_sb_pushu(umn_sb_t *sb, unsigned long v, int enc);   /* string encode a number */
int umn_sb_pushf(umn_sb_t *sb, const char *format, ...) __attribute__((format(printf, 2, 3)));

typedef UMN_SLAB_T(char) umn_arena_t;
void *umn_arena_alloc(umn_arena_t *arena, size_t size);
void *umn_arena_alloc(umn_arena_t *arena, size_t size);

#ifdef UMN_UTILS_IMPLEMENTATION
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
    for (i = slab->count; (size_t)i < slab->capacity; i++)
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

static inline char umn_format__flag(char c)
{
    switch (c)
    {
    case '0':
    case '+':
    case '-':
    case ' ':
    case '#':
        return c;
    default:
        return '\0';
    }
}

static inline char umn_format__conversion(char c)
{
    switch (c)
    {
    case '%':
    case 'c':
    case 's':
    case 'd':
    case 'i':
    case 'B':
    case 'b':
    case 'o':
    case 'X':
    case 'x':
    case 'u':
    case 'F':
    case 'f':
    case 'E':
    case 'e':
    case 'A':
    case 'a':
    case 'G':
    case 'g':
    case 'n':
    case 'p':
        return c;
    default:
        return '\0';
    }
}

static inline const char *umn_format__modifier(const char *s, int *result)
{
    switch (*s)
    {
    case 'h':
    case 'l':
        if (*s == *(s + 1))
            s++;
    case 'j':
    case 't':
    case 'z':
    case 'L':
        *result = (*s) * (*s == *(s - 1) ? -1 : 1);
        return s + 1;
    }

    return NULL;
}

static inline const char *umn_format__int_or_star(const char *s, int *result)
{
    if (*s == '*')
    {
        *result = -1;
        return s + 1;
    }

    char *end = (char *)s;
    while (umn_isdigit(*end))
        end++;

    if (end > s)
    {
        *result = strtoul(s, &end, 10);
        return end;
    }

    return NULL;
}

/* https://en.cppreference.com/c/io/fprintf */
static umn_format_commands_t umn_format_parse(const char *format)
{
    umn_format_commands_t commands = {.capacity = ARRAY_LEN(commands.items)};

    umn_slice_push(commands, ((umn_format_command_t){.s = format}));

    const char *s, *tmp, *init;
    for (s = format; *s; s++)
    {

        if (*s != '%')
        {
            umn_slice_last(commands).length += 1;
            continue;
        }

        if (umn_slice_last(commands).length) /* if there is data commit the thing ... */
            umn_slice_push(commands, ((umn_format_command_t){0}));
        else /* otherwise reuse the command */
            umn_slice_last(commands) = (umn_format_command_t){0};

        init = s, s++;
        umn_slice_last(commands).kind = 1;

        if ((umn_slice_last(commands).flag = umn_format__flag(*s)))
            s++;

        if ((tmp = umn_format__int_or_star(s, &umn_slice_last(commands).width)))
            s = tmp, umn_slice_last(commands).width_set = 1;

        if (*s == '.')
        {
            s++;
            if ((tmp = umn_format__int_or_star(s, &umn_slice_last(commands).precision)))
                s = tmp, umn_slice_last(commands).precision_set = 1;
        }

        if ((tmp = umn_format__modifier(s, &umn_slice_last(commands).modifier)))
            s = tmp;

        if (!(umn_slice_last(commands).conversion = umn_format__conversion(*s)))
        {
            /* failed */
            umn_slice_last(commands) = (umn_format_command_t){
                .kind = -0,
                .length = (unsigned)(s - init) + 1,
                .s = init,
            };

            if (*s == '\0')
                break;
        }
        else
        { /* success */
            umn_slice_push(commands, ((umn_format_command_t){
                                         .kind = 0,
                                         .s = s + 1,
                                         .length = 0,
                                     }));
        }
    }

    return commands;
}

int umn_sb_pushsn(umn_sb_t *sb, const char *s, size_t len)
{
    size_t base = sb->count;

    sb->count += len;
    if ((sb->count + 1) > sb->capacity)
        return (sb->count + 1) - sb->capacity;

    memcpy(sb->items + base, s, len);

    sb->items[sb->count] = '\0';
    return 0;
}

int umn_sb_pushs(umn_sb_t *sb, const char *s)
{
    return umn_sb_pushsn(sb, s, strlen(s));
}

int umn_sb_pushc(umn_sb_t *sb, char v)
{
    size_t count = sb->count + 1;
    if ((count + 1) > sb->capacity)
        return (count + 1) - sb->capacity;

    sb->items[sb->count++] = v;
    sb->items[sb->count] = '\0';

    return 0;
}

int umn_sb_pushcc(umn_sb_t *sb, char v, size_t __count)
{
    size_t count = sb->count + __count;
    if ((count + 1) > sb->capacity)
        return (count + 1) - sb->capacity;

    while (sb->count < count)
        sb->items[sb->count++] = v;

    sb->items[sb->count] = '\0';

    return 0;
}

int umn_sb_pushu(umn_sb_t *sb, unsigned long v, int enc)
{
    size_t count, base = sb->count;

    /* +1 one to prevent overflow for binary thing ... */
    char internal_buffer[sizeof(v) * 8 + 1] = {0};
    char *s = internal_buffer + sizeof(internal_buffer);

    static const char hex_values[] = "0123456789ABCDEF";
    char to_lower = enc >> 8 > 0 ? 0 : 32;

    switch (enc & 0XFF)
    {
    case 2:
        for (; v; v >>= 1)
            *(--s) = '0' + (v & 0x1);
        break;

    case 8:
        for (; v; v >>= 3)
            *(--s) = '0' + (v & 0x7);
        break;

    case 16:

        for (; v; v >>= 4)
            *(--s) = hex_values[(v & 0XF)] | to_lower;
        break;

    default:
        for (; v; v /= 10)
            *(--s) = '0' + (v % 10);
    }

    /* compute the count */
    count = base + (sizeof(internal_buffer) - (s - internal_buffer));

    if (count == base) /* hack for doing the thing ... */
        count += 1, *(--s) = '0';

    if ((count + 1) > sb->capacity)
        return (count + 1) - sb->capacity;

    sb->count = count;
    sb->items[sb->count] = '\0';

    /* copy from internal buffer to destination */
    for (size_t j = base; j < count; j++)
        sb->items[j] = *(s++);

    return 0;
}

static uintmax_t umn_sb_pushf__unsigned(int modifier, uintmax_t v)
{
    if (modifier == -'h')
        return (unsigned char)v;
    else if (modifier == 'h')
        return (unsigned short)v;
    else if (modifier == 'l')
        return (unsigned long)v;
    else if (modifier == -'l')
        return (unsigned long long)v;
    else if (modifier == 'j')
        return (uintmax_t)v;
    else if (modifier == 'z' || modifier == 't')
        return (size_t)v;

    return (unsigned int)v;
}

static intmax_t umn_sb_pushf__signed(int modifier, intmax_t v)
{
    if (modifier == -'h')
        return (char)v;
    else if (modifier == 'h')
        return (short)v;
    else if (modifier == 'l')
        return (long)v;
    else if (modifier == -'l')
        return (long long)v;
    else if (modifier == 'j')
        return (intmax_t)v;
    else if (modifier == 'z' || modifier == 't')
        return (ptrdiff_t)v;

    return (int)v;
}

int umn_sb_pushf(umn_sb_t *sb, const char *format, ...)
{
    umn_format_commands_t commands = umn_format_parse(format);
    umn_format_command_t command;

    char sign_char;
    uintmax_t v;

    umn_sb_t value_sb = {0}, prefix_sb = {0};
    char value_buffer[sizeof(uintmax_t) * 8], prefix_buffer[8]; /* account for prefix */
    value_sb.capacity = sizeof(value_buffer), value_sb.items = value_buffer;
    prefix_sb.capacity = sizeof(prefix_buffer), prefix_sb.items = prefix_buffer;

    /* compute output buffer size first ...  */
    va_list args;
    va_start(args, format);

    for (size_t i = 0; i < commands.count; i++)
    {
        command = umn_slice_at(commands, i);

        if (command.kind == 0)
        {
            umn_sb_pushsn(sb, command.s, command.length);
            continue; /* CONTINUE */
        }

        if (command.width < 0)
            command.width = va_arg(args, int);
        if (command.precision < 0)
            command.precision = va_arg(args, int);

        if (command.conversion == '%')
            umn_sb_pushc(sb, '%');
        else if (command.conversion == 'c')
        {
            UMN_ASSERT(command.modifier != 'l');

            /* prepend */
            if ((size_t)command.width > 1 && command.flag != '-')
                umn_sb_pushcc(sb, ' ', command.width - 1);

            umn_sb_pushc(sb, (char)(va_arg(args, int)));

            if ((size_t)command.width > 1 && command.flag == '-')
                umn_sb_pushcc(sb, ' ', command.width - 1);
        }
        else if (command.conversion == 's')
        {
            UMN_ASSERT(command.modifier != 'l');

            const char *p = va_arg(args, const char *);
            size_t len = command.precision ? command.precision : umn_strlen(p);

            /* prepend */
            if ((size_t)command.width > len && command.flag != '-')
                umn_sb_pushcc(sb, ' ', command.width - len);

            umn_sb_pushsn(sb, p, len);

            /* append */
            if ((size_t)command.width > len && command.flag == '-')
                umn_sb_pushcc(sb, ' ', command.width - len);
        }
        else
        {
            value_sb.count = prefix_sb.count = sign_char = 0;
            if (command.conversion == 'i' || command.conversion == 'd')
            {
                intmax_t tv = umn_sb_pushf__signed(command.modifier, va_arg(args, intmax_t));

                if (tv < 0)
                    v = tv * -1, sign_char = '-';
                else
                    v = tv;

                command.conversion = 'u';
            }
            else
                v = umn_sb_pushf__unsigned(command.modifier, va_arg(args, uintmax_t));

            if (command.precision_set && command.precision == 0 && v == 0)
                continue; /* skip this value */
            else if (command.precision_set == 0)
                command.precision = 1;

            switch (command.conversion)
            {
            case 'o':
                if (command.flag == '#' && v)
                    umn_sb_pushc(&prefix_sb, '0');
                umn_sb_pushu(&value_sb, v, 8);
                break;

            case 'b':
            case 'B':
                if (command.flag == '#' && v)
                    umn_sb_pushc(&prefix_sb, '0'), umn_sb_pushc(&prefix_sb, (char)command.conversion);
                umn_sb_pushu(&value_sb, v, 2);
                break;

            case 'x':
            case 'X':
                if (command.flag == '#' && v)
                    umn_sb_pushc(&prefix_sb, '0'), umn_sb_pushc(&prefix_sb, (char)command.conversion);
                umn_sb_pushu(&value_sb, v, 16 | ((command.conversion == 'X') << 8));
                break;

            case 'u':
                if ((!sign_char && command.flag == ' ') || command.flag == '+')
                    sign_char = '+';
                if (sign_char)
                    umn_sb_pushc(&prefix_sb, sign_char);
                umn_sb_pushu(&value_sb, v, 10);
                break;

            default:
                printf("AAGH %c\n", command.modifier);
                UMN_TODO("handle conversion");
            }

            /* do the thing etc ... */

            size_t width = prefix_sb.count + (((size_t)command.precision > value_sb.count) ? command.precision : value_sb.count);

            /* prepend */
            if ((size_t)command.width > width && command.flag != '-')
                umn_sb_pushcc(sb, ' ', command.width - width);

            umn_sb_pushsn(sb, prefix_sb.items, prefix_sb.count);

            /* pad out the numeric value ... */
            if (value_sb.count < (size_t)command.precision)
                umn_sb_pushcc(sb, '0', command.precision - value_sb.count);

            umn_sb_pushsn(sb, value_sb.items, value_sb.count);

            /* append */
            if ((size_t)command.width > width && command.flag == '-')
                umn_sb_pushcc(sb, ' ', command.width - width);
        }
    }
    va_end(args);

    if ((sb->count + 1) > sb->capacity)
        return (sb->count + 1) - sb->capacity;

    return 0;
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
        umn_slice_at((*arena), -1).items = malloc(umn_slice_at((*arena), -1).capacity);
    }

    data = umn_slice_at((*arena), -1).items + umn_slice_at((*arena), -1).count;
    umn_slice_at((*arena), -1).count += size;
    return data;
}
void umn_arena_free(umn_arena_t *arena)
{
    umn_slab_free((*arena), NULL);
}

#endif
#endif
