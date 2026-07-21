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

#endif /* UMN_CORE_IMPLEMENTATION */
#endif /* UMN_CORE_H */
