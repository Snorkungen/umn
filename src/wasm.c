// process.c

#define UMN_CORE_NOLIBC

__attribute__((export_name("process_string"))) int process_string(const char *str, int length)
{
    // Example: Count the number of 'a' characters
    int count = 0;
    for (int i = 0; i < length; i++)
    {
        if (str[i] == 'a')
        {
            count++;
        }
    }
    return count;
}

// Helper to reset memory if using a simple bump allocator
__attribute__((export_name("reset_memory"))) void reset_memory();

#define UMN_LEXER_IMPLEMENTATION
#define UMN_CORE_IMPLEMENTATION

#include "umn/lexer.h"
#include "umn/expr.h"

__attribute__((export_name("tempalloc"))) void *tempalloc(size_t size)
{
    static void *ptr = NULL;
    return (ptr = umn_memrealloc(ptr, size));
}

static umn_Symbol umn_notation_symbols[] = {
    {"("},
    {")", .flags = UMN_SF_BARRR},
    {",", .flags = UMN_SF_BARRR},

    /* this needs a better way of encoding the the symbol and stuff */
    /* currently the expr parse relies on the fact that the attrs have a precedence which is defined as something */
    {"~", .flags = UMN_SF_UNARY_L},
    {"<<", .flags = UMN_SF_BINOP, .data = 0x80},
    {">>", .flags = UMN_SF_BINOP, .data = 0x80},
    {"&", .flags = UMN_SF_BINOP, .data = 0x80},
    {"^", .flags = UMN_SF_BINOP, .data = 0x80},
    {"|", .flags = UMN_SF_BINOP, .data = 0x80},
};

__attribute__((export_name("count_tokens"))) int count_tokens(const char *str)
{
    umn_Lexer lexer = {
        .data = str,
        .symbols = {
            .count = ARRAY_LEN(umn_notation_symbols),
            .items = umn_notation_symbols,
        },
    };

    umn_Token token;

    int count = 0;
    while (umn_lexer_next(&lexer, &token) == 0 && token.kind)
        count++;

    return count;
}