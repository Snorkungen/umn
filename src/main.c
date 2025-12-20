/* Snorkungen 2025 umn for the next year */

#define UMN_LEXER_IMPL 1
#include "umn/lexer.h"
#include "umn/utils.h"
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

typedef UMN_SLICE_T(char) umn_sb_t;
int umn_sb_appends(umn_sb_t *sb, const char *s)
{
    size_t len = strlen(s);
    umn_slice_reserve((*sb), (len + 1));
    memcpy(sb->items + sb->count, s, len);
    sb->count += len;
    sb->items[sb->count] = '\0';
    return len;
}
int umn_sb_appendc(umn_sb_t *sb, const char v)
{
    umn_slice_reserve((*sb), (2));
    sb->items[sb->count] = v;
    sb->items[++sb->count] = '\0';
    return 1;
}

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
        .data_len = sb.count,
        .symbols = symbols,
        .symbol_count = ARRAY_LEN(symbols),
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

static umn_Symbol umn_notation_symbols[] = {
    {","},
    {"("},
    {")"},

    /* this needs a better way of encoding the the symbol and stuff */
    /* currently the expr parse relies on the fact that the attrs have a precedence which is defined as something */
    {"<<", .attrs.data = 10},
    {">>", .attrs.data = 10},
    {"&", .attrs.data = 10},
    {"^", .attrs.data = 10},
    {"|", .attrs.data = 10},
};

uint64_t compute_node(umn_Lexer *lexer, umn_PNode *node, umn_PNode **err_node)
{
    /* TODO: how do i's indicate an error ...*/
    /* could just exfiltrate bya assigning onto som kind of err node */
    /* this is ugly but I would pressume it to work */
    if (err_node && *err_node)
        return 0;

    assert(node);
    uint64_t value, lvalue, rvalue;
    if (node->token.kind == UMN_KINTEGER)
        return umn_token_readi(lexer, &node->token);
    else if (node->token.kind & UMN_KBINOP && node->lvalue && node->rvalue)
    {
        lvalue = compute_node(lexer, node->lvalue, err_node);
        rvalue = compute_node(lexer, node->rvalue, err_node);

        if (NULL)
            ;
        else if (umn_token_issymbol(lexer, &node->token, "<<"))
            value = lvalue << rvalue;
        else if (umn_token_issymbol(lexer, &node->token, ">>"))
            value = lvalue >> rvalue;
        else if (umn_token_issymbol(lexer, &node->token, "&"))
            value = lvalue & rvalue;
        else if (umn_token_issymbol(lexer, &node->token, "^"))
            value = lvalue ^ rvalue;
        else if (umn_token_issymbol(lexer, &node->token, "|"))
            value = lvalue | rvalue;

        return value;
    }

    if (err_node)
        *err_node = node;

    UMN_TODO("handle differing values");
}

int main(int argc, char **argv)
{
    /* please do not look into this function */
    /* NOTE: leaking memory of the string ... */
    Conf config = init_conf(argc, argv);
    /* construct the lexer ... */

    /* allocator */
    umn_expr_pnode_slab_t nodes = {0};

    umn_Token token;
    umn_Lexer lexer = config.lexer;
    lexer.symbols = umn_notation_symbols;
    lexer.symbol_count = ARRAY_LEN(umn_notation_symbols);

    umn_PNode *node;

    UMN_SLICE_T(struct {umn_PNode *node; uint64_t value; })
    computed_values = {0};

    while ((node = umn_expr_parse(&nodes, &lexer)))
    {
        if ((node->token.kind & UMN_KERR) && node->lvalue != NULL) /* this is such a hack */
        {
            umn_PNode *tmp = node->lvalue;
            umn_lexer_give(&lexer, &node->token);
            umn_slab_drop(nodes, node);
            node = tmp;
        }

        if (node->token.kind & UMN_KERR)
            UMN_TODO("handle errors");

        /* so i would compute the value here ... */
        umn_PNode *err_node = NULL;

        umn_slice_reserve(computed_values, 1);
        computed_values.items[computed_values.count].node = node;
        computed_values.items[computed_values.count].value = compute_node(&lexer, node, &err_node);
        computed_values.count++;

        if (err_node)
        {
            UMN_TODO("handle errors");
        }

        /* read separating commas and stuff  */
        while (umn_lexer_peek(&lexer, &token) == 0 && (umn_token_issymbol(&lexer, &token, ",")))
            umn_lexer_take(&lexer, &token);
    }

    /* iterate over the thing an print the values S*/
    for (int i = 0; i < computed_values.count; i++)
    {
        uint64_t value = umn_slice_at(computed_values, i).value;
        umn_PNode *node = umn_slice_at(computed_values, i).node;

        umn_pnode_print_recurse(&lexer, node);
        printf(" = ");

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
                print_bin(value);
                break;
            case Enc_Oct:
                printf("%#lo", value);
                break;
            case Enc_Hex:
                printf("%#lx", value);
                break;
            case Enc_Dec:
                printf("%lu", value);
                break;
            default:
                abort();
            }
        }
        putchar('\n');
    }

    umn_slab_free(nodes, nodes.items ? nodes.items->items : NULL);
    free((char *)config.lexer.data);
    /* free the computed values slice */
    free(computed_values.items);

    return 0;
}
