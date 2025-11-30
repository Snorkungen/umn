
#include <assert.h>
#define UMN_LEXER_IMPL
#include "./umn-lexer.h"

#if 1
typedef struct
{
    size_t length, capacity;
    char *data;
} umn_Sb;

typedef struct
{
    size_t count, capacity;
    void *items;
} umn_Da;
int umn_da_reserve(umn_Da *da, size_t size, size_t count)
{
    if ((da->count + count) < da->capacity)
        return 0;

    da->capacity = da->capacity * 2 + count;
    da->items = realloc(da->items, da->capacity * size);
    assert(da->items);
    return 0;
}

int umn_sb_appendc(umn_Sb *sb, const char src)
{
    umn_da_reserve((umn_Da *)sb, 1, 2);
    sb->data[sb->length] = src;
    sb->data[++sb->length] = '\0';
    return 0;
}
int umn_sb_append(umn_Sb *sb, const char *src)
{
    int len = strlen(src);

    umn_da_reserve((umn_Da *)sb, 1, len + 1);

    strncpy(sb->data + sb->length, src, sb->capacity - sb->length - 1);
    sb->length += len;
    sb->data[sb->length] = '\0';
    return 0;
}

typedef struct
{
    umn_Token token;
    uint64_t value;
} Expr;
typedef struct
{
    size_t count, capacity;
    Expr *items;
} Exprs;

Expr *exprs_alloc(Exprs *exprs)
{
    umn_da_reserve((umn_Da *)exprs, sizeof(*exprs->items), 1);
    return &exprs->items[exprs->count++];
}
#endif

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

/* NOTE: this function does not output the correct result, values are not big-endian which is expected */
int dec2bin(char *dest, size_t dsize, uint64_t v)
{
    size_t n = 0, offset = 0;
    if (n + 2 < dsize)
    {
        dest[n++] = '0';
        dest[n++] = 'b';
    }

    /* move offset until first set bit is found */
    for (offset = 0; (v & ((0xffULL << 7 * 8)) >> ((offset) * 8)) == 0; offset++)
    {
        ; /* noop the above statement does some cursed B.S. */
    }

    offset = offset * 8; /* multiply byte offset to bits */

    /* move offset untill it finds its first byte with a 1*/
    while (offset <= sizeof(v) * 8)
    {
        if (v & (1ULL << (sizeof(v) * 8 - (++offset))))
            break;
    }

    while (offset <= sizeof(v) * 8 && n + 1 < dsize)
    {
        dest[n++] = v & (1ULL << (sizeof(v) * 8 - offset++)) ? '1' : '0';
    }
    return 0;
}

typedef enum
{
    Enc_Unknown,
    Enc_Bin,
    Enc_Oct,
    Enc_Dec,
    Enc_Hex,
} Encs;

static umn_Symbol options_symbols[] = {
    {"-"},
    {"d", .attrs.data = Enc_Dec},
    {"b", .attrs.data = Enc_Bin},
    {"o", .attrs.data = Enc_Oct},
    {"x", .attrs.data = Enc_Hex},

    {"--"},
    {"decimal", .attrs.data = Enc_Dec},
    {"binary", .attrs.data = Enc_Bin},
    {"octal", .attrs.data = Enc_Oct},
    {"hex", .attrs.data = Enc_Hex},
};

static umn_Symbol symbols[] = {
    {","}, /* expression separator */

    /* The shell already uses this symbols, what could we replace this with? */ {"~"},  /* unary */
    /* The shell already uses this symbols, what could we replace this with? */ {"|"},  /* binop OR */
    /* The shell already uses this symbols, what could we replace this with? */ {"&"},  /* binop AND */
    /* The shell already uses this symbols, what could we replace this with? */ {"<<"}, /* binop lshift */
    /* The shell already uses this symbols, what could we replace this with? */ {">>"}, /* binop rshift */
};

int main(int argc, char **argv)
{
    umn_Sb sb = {0};
    Exprs exprs = {0};
    Expr *expr;
    umn_Token token, next_token;

    int base_enc_map[] = {
        [Enc_Dec] = 10,
        [Enc_Bin] = 2,
        [Enc_Oct] = 8,
        [Enc_Hex] = 16,
    };
    bool encs[Enc_Hex + 1] = {0};

    for (int i = 1; i < argc; i++)
    {
        umn_sb_append(&sb, argv[i]);
        umn_sb_appendc(&sb, ' ');
    }

    umn_Lexer lexer = {.data = sb.data, .data_len = sb.length};
    /* 1st read the options */
    lexer.symbols = options_symbols;
    lexer.symbol_count = ARRAY_LEN(options_symbols);
    while (umn_lexer_peek(&lexer, &token) == 0 && (token.kind & (~UMN_KERR)) != UMN_KEOF)
    {
        /* -dx or -d -x */
        uint64_t expected_begin = (token.begin + token.length);

        if (umn_token_issymbol(&lexer, &token, "-"))
        {
            umn_lexer_take(&lexer, &token);

            while (umn_lexer_peek(&lexer, &next_token) == 0 && (expected_begin == next_token.begin) && next_token.kind == UMN_KSYMBOL)
            {
                expected_begin = next_token.begin + token.length;
                umn_lexer_take(&lexer, &next_token);

                if (next_token.d.symbol.data == 0)
                {
                    /* set some error condition */
                    break;
                }

                assert(next_token.d.symbol.data < ARRAY_LEN(encs));
                encs[next_token.d.symbol.data] = true;
            }
        }
        else if (umn_token_issymbol(&lexer, &token, "--"))
        {
            umn_lexer_take(&lexer, &token);
            if (umn_lexer_peek(&lexer, &next_token) == 0 && expected_begin == next_token.begin && next_token.kind == UMN_KSYMBOL)
            {
                if (next_token.d.symbol.data == 0)
                    continue;

                assert(next_token.d.symbol.data < ARRAY_LEN(encs));
                encs[next_token.d.symbol.data] = true;
            }
            umn_token_print(&lexer, &next_token);
        }
        else
        {
            break; /* do no damage */
        }
    }

    /* 2nd parse the expressions and stuff */
    lexer.symbols = symbols;
    lexer.symbol_count = ARRAY_LEN(symbols);

    while (umn_lexer_next(&lexer, &token) == 0 && (token.kind & (~UMN_KERR)) != UMN_KEOF)
    {
        int64_t v;
        if (token.kind == UMN_KSTRING && token.length == 1)
        {
            v = (*(lexer.data + token.begin));
            if (v >= 0x80)
                continue; /* IGNORE */
        }
        else if (token.kind == UMN_KINTEGER)
            v = umn_token_readi(&lexer, &token);
        else
        {
            continue; /* IGNORE */
        }

        expr = exprs_alloc(&exprs);
        memcpy(&expr->token, &token, sizeof(token));
        expr->value = v;
    }

    /* 3rd default to decimal */
    bool no_encs = !(encs[Enc_Hex] || encs[Enc_Oct] || encs[Enc_Bin]);
    if (no_encs)
    {
        encs[Enc_Dec] = true;
    }
    /* 4th output the values */
    for (int enc = Enc_Unknown; enc <= Enc_Hex; enc++)
    {
        if (!encs[enc])
            continue;

        printf("%2d:  ", base_enc_map[enc]);
        for (int i = 0; i < exprs.count; i++)
        {
            expr = exprs.items + i;
            if (i > 0)
                printf(", ");

            static char input[100] = {0};
            static char s[100] = {0};
            /* NOTE theese functions are returning some FS */
            if (enc == Enc_Dec)
                snprintf(s, sizeof(s) - 1, "%lu", expr->value);
            else if (enc == Enc_Bin)
                dec2bin(s, sizeof(s) - 1, expr->value);
            else if (enc == Enc_Oct)
                snprintf(s, sizeof(s) - 1, "%#lo", expr->value);
            else if (enc == Enc_Hex)
                snprintf(s, sizeof(s) - 1, "0x%lx", expr->value);

            umn_token_strncpy(&lexer, &expr->token, input, sizeof(input));

            if ((token.kind & UMN_KNUMERIC) == 0)
            {
                printf("'%s'=%s", input, s);
            }
            else
            {
                printf("%s=%s", input, s);
            }
        }
        printf("\n");
    }

    return 0;
}
