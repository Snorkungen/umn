#ifndef UMN_CORE_H
#define UMN_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef UMN_DEF
#define UMN_DEF static
#endif

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define umn_assert(expr) ((expr) ? (void)0 : umn_panic("assert failed", __FILE__, __LINE__))

#define UMN_TODO(msg)                                     \
    do                                                    \
    {                                                     \
        umn_panic("TODO(" msg ")\n", __FILE__, __LINE__); \
    } while (0) //  printf("%s:%d: TODO(%s)\n", __FILE__, __LINE__, msg);

/* TODO: Move the sb, and sb_format logic to the core since some core function should leverage the string, builder.
            What I think is that will probably happen is I move all the utils logic into the core file for consistency */

__attribute__((noreturn)) UMN_DEF void umn_panic(const char *message, const char *file, int line);

/* building blocks */
static void umn_write_to_stdout(const char *message, const char *file, int line);
static void umn_write_to_stderr(const char *message, const char *file, int line);

UMN_DEF void *umn_memcpy(void *restrict dst, const void *restrict src, size_t n);
UMN_DEF void *umn_memmove(void *dst, const void *src, size_t n);
UMN_DEF void *umn_memset(void *dst, int c, size_t n);
UMN_DEF int umn_memcmp(const void *s1, const void *s2, size_t n);

UMN_DEF size_t umn_strlen(const char *s);
UMN_DEF int umn_strncmp(const char *s1, const char *s2, size_t n);
UMN_DEF char *umn_strncpy(char *dst, const char *src, size_t n);
UMN_DEF double umn_strtod(const char *nptr, char **endptr);
UMN_DEF long umn_strtol(const char *nptr, char **endptr, int base);

/* ctypes is a project */
UMN_DEF int umn_isdigit(int c);  /* is a digit 0 - 9 */
UMN_DEF int umn_isbdigit(int c); /* is a binary digit 0 or 1*/
UMN_DEF int umn_isodigit(int c); /* is an octal digit 0 - 7 */
UMN_DEF int umn_isxdigit(int c); /* is a hexadecimal digit */
UMN_DEF int umn_isspace(int c);
UMN_DEF int umn_isupper(int c);
UMN_DEF int umn_islower(int c);
UMN_DEF int umn_toupper(int c);
UMN_DEF int umn_tolower(int c);

/* UMN SLICE MACROS -- BEGIN */

#define UMN_SLICE_T(Type)       \
    struct                      \
    {                           \
        size_t count, capacity; \
        Type *items;            \
    }

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

/* UMN SLICE MACROS -- END */

/* UMNs global allocator, strategy, allow for a stack only program, if I wanted */
UMN_DEF void umn_meminit(void *buffer, size_t size);
UMN_DEF void *umn_memalloc(size_t size);
UMN_DEF void *umn_memrealloc(void *ptr, size_t size);
UMN_DEF void umn_memfree(void *ptr);

/* UMN SB & FORMAT -- BEGIN */

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
} umn_format_t;

UMN_DEF const char *umn_format_get(const char *fmt, umn_format_t *command);

typedef UMN_SLICE_T(char) umn_sb_t;
UMN_DEF int umn_sb_pushc(umn_sb_t *sb, char c);                                   /* push a single char */
UMN_DEF int umn_sb_pushcn(umn_sb_t *sb, char c, size_t count);                    /* push a given char n times */
UMN_DEF int umn_sb_pushsn(umn_sb_t *sb, const char *s, size_t len);               /* push a string up to n*/
UMN_DEF int umn_sb_pushs(umn_sb_t *sb, const char *s);                            /* push a string */
UMN_DEF int umn_sb_pushu(umn_sb_t *sb, unsigned long v, int enc);                 /* string encode a number */
UMN_DEF int umn_sb_push_format(umn_sb_t *sb, umn_format_t command, va_list args); /* push a format command */
UMN_DEF int umn_sb_pushf(umn_sb_t *sb, const char *format, ...) __attribute__((format(printf, 2, 3)));

/*

    while (umn_sb_grow(arena, sb, umn_sb_pushs(sb, "Hello, E, Jon"))) {};

*/

// UMN_DEF int umn_sb_pushf(umn_sb_t *sb, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* UMN SB & FORMAT -- END */

#ifdef UMN_CORE_IMPLEMENTATION

#ifndef UMN_CORE_NOLIBC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UMN_DEF void umn_panic(const char *message, const char *file, int line)
{
    fprintf(stderr, "panic: %s:%d %s\n", file, line, message);
    abort();
}

UMN_DEF inline size_t umn_strlen(const char *s)
{
    return strlen(s);
}

UMN_DEF inline int umn_strncmp(const char *s1, const char *s2, size_t n)
{
    return strncmp(s1, s2, n);
}

UMN_DEF inline char *umn_strncpy(char *dst, const char *src, size_t n)
{
    return strncpy(dst, src, n);
}

UMN_DEF inline double umn_strtod(const char *nptr, char **endptr)
{
    return strtod(nptr, endptr);
}

UMN_DEF inline long umn_strtol(const char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}

static void umn_write_to_stdout(const char *message, const char *file, int line)
{
    fputs(message, stdout);
}

static void umn_write_to_stderr(const char *message, const char *file, int line)
{
    fputs(message, stderr);
}

#else

__attribute__((noreturn)) UMN_DEF void umn_panic(const char *message, const char *file, int line)
{
#if defined(__wasm__)
    __asm__ __volatile__("unreachable"); // forces a trap/unreachable
#else
    // not wasm: left as a best-effort fallback
    // (you can replace with abort() if you want)
    for (;;)
    {
    }
#endif
}

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s)
    {
        for (size_t i = 0; i < n; ++i)
            d[i] = s[i];
    }
    else
    {
        for (size_t i = n; i != 0; --i)
            d[i - 1] = s[i - 1];
    }

    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char uc = (unsigned char)c;
    for (size_t i = 0; i < n; ++i)
        d[i] = uc;
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    for (size_t i = 0; i < n; ++i)
    {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

#include <limits.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) ((x) - ONES & ~(x) & HIGHS)

/* https://git.musl-libc.org/cgit/musl/tree/src/string/strlen.c */
UMN_DEF size_t umn_strlen(const char *s)
{
    const char *a = s;
#ifdef __GNUC__
    typedef size_t __attribute__((__may_alias__)) word;
    const word *w;
    for (; (uintptr_t)s % ALIGN; s++)
        if (!*s)
            return s - a;
    for (w = (const void *)s; !HASZERO(*w); w++)
        ;
    s = (const void *)w;
#endif
    for (; *s; s++)
        ;
    return s - a;
}

UMN_DEF int umn_strncmp(const char *s1, const char *s2, size_t n)
{
    const unsigned char *l = (void *)s1, *r = (void *)s2;
    if (!n--)
        return 0;
    for (; *l && *r && n && *l == *r; l++, r++, n--)
        ;
    return *l - *r;
}

UMN_DEF inline char *umn_strncpy(char *dst, const char *src, size_t n)
{
    if (dst == NULL || src == NULL || n == 0)
        return dst;

    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];

    for (; i < n; i++)
        dst[i] = '\0';

    return dst;
}

UMN_DEF inline double umn_strtod(const char *nptr, char **endptr)
{
    umn_panic("umn_strtod: has not been implemented", __FILE__, __LINE__);
}

static void umn_write_to_stdout(const char *message, const char *file, int line)
{
    umn_panic("umn_write_to_stdout: has not been implemented", __FILE__, __LINE__);
}

static void umn_write_to_stderr(const char *message, const char *file, int line)
{
    umn_panic("umn_write_to_stderr: has not been implemented", __FILE__, __LINE__);
}

#endif /* UMN_CORE_NOLIBC */

UMN_DEF inline void *umn_memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    return memcpy(dst, src, n);
}

UMN_DEF inline void *umn_memmove(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

UMN_DEF inline void *umn_memset(void *dst, int c, size_t n)
{
    return memset(dst, c, n);
}

UMN_DEF inline int umn_memcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

/* UMN CTYPES -- BEGIN */

UMN_DEF inline int umn_isspace(int c)
{
    return (c == ' ' || c == '\t' ||
            c == '\n' || c == '\v' ||
            c == '\f' || c == '\r');
}

UMN_DEF inline int umn_isdigit(int c) { return ((unsigned)(c) - '0') < 10; }
UMN_DEF inline int umn_isbdigit(int c) { return ((unsigned)(c) - '0') <= 1; }
UMN_DEF inline int umn_isodigit(int c) { return ((unsigned)(c) - '0') <= 7; }
UMN_DEF inline int umn_isxdigit(int c) { return umn_isdigit(c) || ((unsigned)c | 32) - 'a' <= ('f' - 'a'); }
UMN_DEF inline int umn_isupper(int c) { return ((unsigned)(c) - 'A') <= ('A' - 'Z'); }
UMN_DEF inline int umn_islower(int c) { return ((unsigned)(c) - 'a') <= ('a' - 'z'); }

UMN_DEF inline int umn_tolower(int c)
{
    if (umn_isupper(c))
        return c | 32;
    return c;
}

UMN_DEF inline int umn_toupper(int c)
{
    if (umn_islower(c))
        return c & 0xdf;
    return c;
}

/* UMN CTYPES -- END */

/* UMN MISC -- BEGIN */

/* UMN MISC -- END */

/* UMN MEM -- BEGIN */

#define UMN_MEMBSIZE 64 /* This is memory allocators block size */

static struct
{
    size_t offset;
    uint8_t *data, *head, *data_end;
} umn__mem = {0};

UMN_DEF void umn_meminit(void *buffer, size_t size)
{
    umn_assert(umn__mem.data == NULL && umn__mem.data_end == NULL);

    umn__mem.offset = 0;

    umn__mem.data = umn__mem.head = buffer;
    umn__mem.data_end = buffer + size;
}

UMN_DEF void umn_memfree(void *ptr)
{
    if (((size_t)umn__mem.head - (size_t)ptr) > umn__mem.offset)
        return;

    umn__mem.head -= umn__mem.offset;
    umn__mem.offset = 0;
}

UMN_DEF void *umn_memalloc(size_t size)
{
    size = ((size + 7) >> 3) << 3; /* ((size + 7) / 8) * 8; */
    size += sizeof(size_t);        /* account for the allocation size and stuff ... */

    if (umn__mem.data_end < (umn__mem.head + size))
        return NULL;

    umn__mem.offset = size;
    umn__mem.head += size;

    memset((void *)(umn__mem.head - size), 0, size);
    *(size_t *)(umn__mem.head - size) = size - sizeof(size_t);
    return (void *)(umn__mem.head - (size - sizeof(size_t)));
}

UMN_DEF void *umn_memrealloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return NULL;

    size = ((size + 7) >> 3) << 3; /* ((size + 7) / 8) * 8; */
    size += sizeof(size_t);        /* account for the allocation size and stuff ... */

    if (((size_t)umn__mem.head - (size_t)ptr) > umn__mem.offset)
    {
        void *data = umn_memalloc(size);
        if ((data) == NULL)
            return NULL;

        memcpy(data, ptr, *((size_t *)ptr - 1) - sizeof(size_t));
        return data;
    }
    else if (size < *((size_t *)ptr - 1))
    {
        *((size_t *)ptr - 1) = size;
        return ptr;
    }
    else
    {
        size_t old_size = *((size_t *)ptr - 1), diff = size - old_size;

        if (umn__mem.head + diff > umn__mem.data_end)
            return NULL;

        memset(umn__mem.head, 0, diff);
        umn__mem.head += diff;
        umn__mem.offset += diff;

        *((size_t *)ptr - 1) = size;

        return ptr;
    }

    return NULL;
}

/* UMN MEM -- END */

/* UMN SB & FORMAT -- BEGIN */

/* https://en.cppreference.com/c/io/fprintf */
UMN_DEF const char *umn_format_get(const char *fmt, umn_format_t *command)
{
    if (fmt == NULL || *fmt == '\0')
        return NULL;

    if (*fmt != '%')
    {
        command->kind = 0;
        command->s = fmt;

        do
        {
            fmt++;
        } while (fmt != '\0' && fmt != '%');

        command->length = (size_t)(fmt - command->s);

        return fmt;
    }

    if (*(fmt + 1) == '%')
    {
        command->kind = 0;
        command->s = fmt;
        command->length = 1;

        return fmt + 2;
    }

    memset(command, 0, sizeof(umn_format_t));

    const char *tmp, *s = fmt + 1;
    command->kind = 1;

    if ((command->flag = umn_format__flag(*s)))
        s++;

    if ((tmp = umn_format__int_or_star(s, &command->width)))
        s = tmp, command->width_set = 1;

    if (*s == '.')
    {
        s++;
        if ((tmp = umn_format__int_or_star(s, &command->precision)))
            s = tmp, command->precision_set = 1;
    }

    if ((tmp = umn_format__modifier(s, &command->modifier)))
        s = tmp;

    if (!(command->conversion = umn_format__conversion(*s)))
    { /* failed, recover by treating this a regular string */

        command->kind = 0;
        command->s = fmt;
        command->length = s - fmt;
    }

    return s + 1;
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
        *result = umn_readu(s, (end - s), 10);
        return end;
    }

    return NULL;
}

UMN_DEF int umn_sb_push_format(umn_sb_t *sb, umn_format_t command, va_list args)
{
    if (command.kind == 0)
        return umn_sb_pushsn(sb, command.s, command.length);

    if (command.width < 0)
        command.width = va_arg(args, int);
    if (command.precision < 0)
        command.precision = va_arg(args, int);

    int overflow = 0;
    char sign_char;
    uintmax_t v;
    umn_sb_t value_sb = {0}, prefix_sb = {0};
    char value_buffer[sizeof(uintmax_t) * 8], prefix_buffer[8]; /* account for prefix */
    value_sb.capacity = sizeof(value_buffer), value_sb.items = value_buffer;
    prefix_sb.capacity = sizeof(prefix_buffer), prefix_sb.items = prefix_buffer;

    if (command.modifier == 'c')
    {
        umn_assert(command.modifier != 'l');

        /* prepend */
        if ((size_t)command.width > 1 && command.flag != '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - 1);

        overflow += umn_sb_pushc(sb, (char)(va_arg(args, int)));

        /* append */
        if ((size_t)command.width > 1 && command.flag == '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - 1);
    }
    else if (command.conversion == 's')
    {
        umn_assert(command.modifier != 'l');

        const char *p = va_arg(args, const char *);
        size_t len = command.precision ? command.precision : umn_strlen(p);

        /* prepend */
        if ((size_t)command.width > len && command.flag != '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - len);

        overflow += umn_sb_pushsn(sb, p, len);

        /* append */
        if ((size_t)command.width > len && command.flag == '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - len);
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
            return 0; /* skip this value */
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
            UMN_TODO("TODO: handle unhandled conversion");
        }

        size_t width = prefix_sb.count + (((size_t)command.precision > value_sb.count) ? command.precision : value_sb.count);

        /* prepend */
        if ((size_t)command.width > width && command.flag != '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - width);

        overflow += umn_sb_pushsn(sb, prefix_sb.items, prefix_sb.count);

        /* pad out the numeric value ... */
        if (value_sb.count < (size_t)command.precision)
            overflow += umn_sb_pushcc(sb, '0', command.precision - value_sb.count);

        overflow += umn_sb_pushsn(sb, value_sb.items, value_sb.count);

        /* append */
        if ((size_t)command.width > width && command.flag == '-')
            overflow += umn_sb_pushcc(sb, ' ', command.width - width);
    }

    return overflow;
}

UMN_DEF int umn_sb_pushcn(umn_sb_t *sb, char c, size_t __count)
{
    size_t count = sb->count + __count;
    if ((count + 1) > sb->capacity)
        return (count + 1) - sb->capacity;

    while (sb->count < count)
        sb->items[sb->count++] = c;

    sb->items[sb->count] = '\0';

    return 0;
}

UMN_DEF inline int umn_sb_pushc(umn_sb_t *sb, char c)
{
    return umn_sb_pushcn(sb, c, 1);
}

UMN_DEF int umn_sb_pushsn(umn_sb_t *sb, const char *s, size_t len)
{
    size_t base = sb->count;

    sb->count += len;
    if ((sb->count + 1) > sb->capacity)
        return (sb->count + 1) - sb->capacity;

    umn_memcpy(sb->items + base, s, len);

    sb->items[sb->count] = '\0';
    return 0;
}

UMN_DEF inline inline int umn_sb_pushs(umn_sb_t *sb, const char *s)
{
    return umn_sb_pushsn(sb, s, umn_strlen(s));
}

UMN_DEF int umn_sb_pushu(umn_sb_t *sb, unsigned long v, int enc)
{
    size_t count, base = sb->count;

    /* +1 one to prevent overflow for binary thing ... */
    char internal_buffer[(sizeof(v) + 1) * 8] = {0};
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

UMN_DEF int umn_sb_pushf(umn_sb_t *sb, const char *format, ...) __attribute__((format(printf, 2, 3)))
{
    int overflow = 0;
    umn_format_t command = {0};

    va_list args;
    va_start(args, format);
    
    while ((format = umn_format_get(format, &command)))
        overflow += umn_sb_push_format(sb, command, args);

    va_end(args);

    return overflow;
}

/* UMN SB & FORMAT -- END */

#endif /* UMN_CORE_IMPLEMENTATION */
#endif /* UMN_CORE_H */
