#ifndef UMN_CORE_H
#define UMN_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef UMN_DEF
#define UMN_DEF
#endif

__attribute__((noreturn)) UMN_DEF void umn_panic(const char *message, const char *file, int line);

UMN_DEF void *umn_memcpy(void *restrict dst, const void *restrict src, size_t n);
UMN_DEF void *umn_memmove(void *dst, const void *src, size_t n);
UMN_DEF void *umn_memset(void *dst, int c, size_t n);
UMN_DEF int umn_memcmp(const void *s1, const void *s2, size_t n);

UMN_DEF size_t umn_strlen(const char *s);
UMN_DEF int umn_strncmp(const char *s1, const char *s2, size_t n);
UMN_DEF char *umn_strncpy(char *dst, const char *src, size_t n);
UMN_DEF double umn_strtod(const char *nptr, char **endptr);
UMN_DEF long umn_strtol(const char *nptr, char **endptr, int base);

/* building blocks */
static void umn_write_to_stdout(const char *message, const char *file, int line);
static void umn_write_to_stderr(const char *message, const char *file, int line);

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

#ifdef UMN_CORE_IMPLEMENTATION
#ifndef UMN_CORE_NOLIBC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noreturn)) UMN_DEF void umn_panic(const char *message, const char *file, int line)
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

#endif /* UMN_CORE_IMPLEMENTATION */
#endif /* UMN_CORE_H */
