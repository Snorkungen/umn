/* Snorkungen 2026 umn for the next year */

#define UMN_LEXER_IMPLEMENTATION
#define UMN_UTILS_IMPLEMENTATION
#define UMN_EXPR_IMPLEMENTATION
#include "umn/expr.h"

typedef enum
{
    Enc_Bin,
    Enc_Oct,
    Enc_Dec,
    Enc_Hex,
    Enc_Last
} Enc;

const char ENC_MAP_S[] = {
    [Enc_Bin] = 'b',
    [Enc_Oct] = 'o',
    [Enc_Dec] = 'd',
    [Enc_Hex] = 'x',
};

const char *ENC_MAP_L[] = {
    [Enc_Bin] = "binary",
    [Enc_Oct] = "octal",
    [Enc_Dec] = "decimal",
    [Enc_Hex] = "hex",
};

typedef struct
{
    umn_Lexer lexer;
    bool output[Enc_Last];
} Conf;

Conf init_conf(int argc, char **argv)
{
    Conf config = {0};

    umn_sb_t sb = {0};
    umn_slice_reserve(sb, 128);
    sb.items[0] = '\0';

    for (int i = 1; i < argc; i++)
    {
        umn_sb_appendc(&sb, ' ');
        umn_sb_appends(&sb, argv[i]);
    }

    umn_Token token;
    static umn_Symbol symbols[] = {{"-"}, {"--"}}; /* What's the syntax */
    umn_Lexer lexer = {
        .data = sb.items,
        .symbols = {
            .count = ARRAY_LEN(symbols),
            .items = symbols,
        },
    };

    /* this magic fuckin loop does stuff etc .. k*/
    while (umn_lexer_peek(&lexer, &token) == 0)
    {
        size_t token_end = token.begin + token.length;

        if (umn_token_issymbol(&lexer, &token, "--"))
        {
            umn_lexer_take(&lexer, &token);
            umn_lexer_peek(&lexer, &token);

            if (token.begin != token_end)
                break; /* this does not matter any more */

            for (int i = 0; i < Enc_Last; i++)
                config.output[i] = config.output[i] || umn_token_litcmp(&lexer, &token, ENC_MAP_L[i]) == 0;

            umn_lexer_take(&lexer, &token);
            continue;
        }
        else if (umn_token_issymbol(&lexer, &token, "-"))
        {
            umn_lexer_take(&lexer, &token);
            umn_lexer_peek(&lexer, &token);

            if (token.begin != token_end || token.kind != UMN_KLITERAL)
                continue; /* let's move on this should error but oh well */

            for (int j = 0; j < token.length; j++)
            {
                for (int i = 0; i < Enc_Last; i++)
                    config.output[i] = config.output[i] || lexer.data[token.begin + j] == ENC_MAP_S[i];
            }

            umn_lexer_take(&lexer, &token);
            continue;
        }

        break;
    }

    /* NOTE: leaking memory, this should just return the lexer */
    config.lexer = lexer;

    /* set default output */
    {
        unsigned int sum = 0;
        for (int i = 0; i < Enc_Last; i++)
            sum += config.output[i];

        if (sum == 0)
            config.output[Enc_Dec] = true;
    }

    return config;
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

void print_bin(uint64_t value)
{
    uint64_t n = value;
    unsigned int j = 0;
    while (n >>= 1)
        j++;

    printf("0b");
    do
    {
        putchar(((value & (1UL << j)) > 0) + '0');
    } while (j-- != 0);
}

/* TODO: support arbitrary precision numbers */
typedef struct
{
    uint64_t value;
} umn_uint_t;

#define umn_uint_byte_count(__n__) ((((sizeof(__n__) * 8) - __builtin_clzl(__n__ | 1)) + 7) / 8)
#define umn_uint(__n__) ((umn_uint_t){.value = __n__})

umn_uint_t compute_node(const umn_Lexer *lexer, const umn_PToken *p, umn_Token *err_token)
{
    /* TODO: how do i's indicate an error ...*/
    /* could just exfiltrate bya assigning onto som kind of err node */
    /* this is ugly but I would pressume it to work */
    if (err_token && err_token->kind)
        return umn_uint(0);

    assert(p);

    umn_uint_t value = {0}, lvalue, rvalue;
    if (p->token.kind == UMN_KINTEGER)
        return umn_uint(umn_token_readu(lexer, &p->token));
    else if (p->token.kind & UMN_KBINOP && p->lvalue && p->rvalue)
    {
        lvalue = compute_node(lexer, p->lvalue, err_token);
        rvalue = compute_node(lexer, p->rvalue, err_token);

        if (NULL)
            ;
        else if (umn_token_issymbol(lexer, &p->token, "<<"))
        {
            lvalue.value <<= rvalue.value;
        }
        else if (umn_token_issymbol(lexer, &p->token, ">>"))
        {
            lvalue.value >>= rvalue.value;
        }
        else if (umn_token_issymbol(lexer, &p->token, "&"))
        {
            lvalue.value &= rvalue.value;
        }
        else if (umn_token_issymbol(lexer, &p->token, "^"))
        {
            lvalue.value ^= rvalue.value;
        }
        else if (umn_token_issymbol(lexer, &p->token, "|"))
        {
            lvalue.value |= rvalue.value;
        }

        return lvalue;
    }
    else if (p->token.kind & UMN_KUNARY_L && p->rvalue && umn_token_issymbol(lexer, &p->token, "~"))
    {
        value = compute_node(lexer, p->rvalue, err_token);
        value.value = ~value.value ^ (~0UL << (umn_uint_byte_count(value.value) * 8));

        return value;
    }

    if (err_token)
        *err_token = p->token;

    UMN_TODO("handle differing values");
}

int main(int argc, char **argv)
{
    umn_PToken_Allocator ptokens = {0};
    umn_Token token, err_token = {0};
    Conf config = init_conf(argc, argv);

    config.lexer.symbols = (umn_Lexer_Symbols){
        .items = umn_notation_symbols,
        .count = ARRAY_LEN(umn_notation_symbols),
    };

    UMN_SLICE_T(struct {const umn_PToken *p; umn_uint_t value; })
    computed_values = {0};

    const umn_PToken *p;

    while ((p = umn_expr_parse(&config.lexer, &ptokens, NULL, NULL)))
    {
        if (p->token.kind & UMN_KERR)
        {
            puts("failed to read the following value: ");
            umn_token_print_error(&config.lexer, &p->token);
            return 1;
        }

        umn_slice_reserve(computed_values, 1);
        computed_values.items[computed_values.count].p = p;
        computed_values.items[computed_values.count].value = compute_node(&config.lexer, p, &err_token);
        computed_values.count++;

        if (err_token.kind)
        {
            umn_token_print(&config.lexer, &err_token);
            UMN_TODO("handle errors");
        }

        /* read separating commas and stuff  */
        while (umn_lexer_peek(&config.lexer, &token) == 0 && (umn_token_issymbol(&config.lexer, &token, ",")))
            umn_lexer_take(&config.lexer, &token);
    }

    /* iterate over the thing an print the values S*/
    char buffer[512] = {0};
    for (int i = 0; i < computed_values.count; i++)
    {
        umn_uint_t value = umn_slice_at(computed_values, i).value;
        p = umn_slice_at(computed_values, i).p;

        printf("%s = ", umn_ptoken_strncpy(&config.lexer, p, buffer, sizeof(buffer)));

        bool touched = false;
        for (Enc e = 0; e < Enc_Last; e++)
        {
            if (!config.output[e])
                continue;

            if (touched)
                printf(", ");
            else
                touched = true;

            switch (e)
            {
            case Enc_Bin:
                print_bin(value.value);
                break;
            case Enc_Oct:
                printf("%#lo", value.value);
                break;
            case Enc_Hex:
                printf("%#lx", value.value);
                break;
            case Enc_Dec:
                printf("%lu", value.value);
                break;
            default:
                abort();
            }
        }
        putchar('\n');
    }

    umn_slab_free(ptokens, ptokens.items ? ptokens.items->items : NULL);

    /* free the computed values slice */
    free((char *)config.lexer.data);
    free(computed_values.items);

    return 0;
}
