#define UMN_LEXER_IMPLEMENTATION
#define UMN_UTILS_IMPLEMENTATION
#define UMN_EXPR_IMPLEMENTATION

#include "umn/expr.h"

uint64_t compute(const umn_Lexer *lexer, umn_PToken *root)
{
    // assert(root);
    if (root == NULL)
        return -1;

    if (root->token.kind == UMN_KINTEGER)
        return umn_token_readi(lexer, &root->token);

    int64_t res, lv = compute(lexer, root->lvalue), rv = compute(lexer, root->rvalue);

    if (umn_token_issymbol(lexer, &root->token, "+"))
        res = lv + rv;
    else if (umn_token_issymbol(lexer, &root->token, "*"))
        res = lv * rv;
    else if (umn_token_issymbol(lexer, &root->token, "**"))
    {
        res = 1;
        for (unsigned int i = 0; i < rv; i++)
            res *= lv;
    }
    else
    {
        UMN_TODO("handle the operator");
    }

    return res;
}

void test_1(umn_PToken_Allocator *ptokens, umn_Lexer *lexer)
{
    char cbuffer[256] = {0};

    const struct
    {
        const char *str;
        int64_t expected;
    } tests[] = {
        {"1 + 2 ** 3 + 4", 13},
        {"(2)", 2},
        {"(1) + (2)", 3},
        {"1 + 2 ** 2 ", 5},
        {"1 + 2 * 10 * 3 ** 4 + 5", 1626},
        {"(1) + (2 * 3 + 1) ** 4 + 5", 2407},
        {"1 + (2) * 3 * 4", 25},
        {"(1) + (2 * 3)", 7},
        {"1 + 2 + (3 * 4) + 5 * 6", 45},
        {"1 + 2 * (3 + 4) * 5", 71},
        {"(1 * 1) + 2 + 3 +4 + 5+ 6 * 1", 21},
    };

    umn_PToken *res;
    for (int i = 0; i < ARRAY_LEN(tests); i++)
    {
        /* soft reset the slab allocator */
        if (ptokens->items)
            umn_slab_drop((*ptokens), ptokens->items->items);

        lexer->position = 0;
        lexer->data = tests[i].str;

        res = umn_expr_parse(ptokens, lexer);

        int64_t value = compute(lexer, res);

        printf("%s = %ld\n",
               umn_ptoken_strncpy(lexer, res, cbuffer, sizeof(cbuffer)),
               value);

        if (value - tests[i].expected)
        {
            printf("\033[31m");
            umn_ptoken_tree_print(lexer, res);
            printf("\033[0m");
            break;
        }
    }

    /* free all nodes  */
    umn_slab_free((*ptokens), ptokens->items->items);
}

static const umn_Symbol umn_expr_symbols__[] = {
    {.s = "."},
    {.s = ",", .flags = UMN_SF_BARRR},
    {.s = "("},
    {.s = ")"},
    {.s = "=", .flags = UMN_SF_BINOP, .data = 16},
    {.s = "+", .flags = UMN_SF_BINOP | UMN_SF_UNARY_L, .data = 18},
    {.s = "*", .flags = UMN_SF_BINOP, .data = 19},
    {.s = "**", .flags = UMN_SF_BINOP, .data = 20},
};
static const umn_Lexer_Symbols umn_expr_symbols = {
    .count = ARRAY_LEN(umn_expr_symbols__),
    .items = umn_expr_symbols__,
};

int main(void)
{
    char cbuffer[256] = {0};
    umn_PToken_Allocator ptokens = {0};
    umn_Lexer lexer = {
        .symbols = umn_expr_symbols,
    };

    umn_PToken *ptoken;

    test_1(&ptokens, &lexer);
    puts("--------------------------");
    /* parse a function and do things ... */
    
    /* f(x) = 2 * x */
    lexer.position = 0;
    // lexer.data = "f(x,) = 2 * x, f(3)"; /* I want this to compute to 6*/
    lexer.data = "f(x, y = 2) = x + 1"; /* I want this to compute to 6*/
    
    ptoken = umn_expr_parse_ext(&ptokens, &lexer, NULL);
    umn_ptoken_tree_print(&lexer, ptoken);

    puts("--------------------------");

    lexer.position = 0;
    lexer.data = "+10 * +++(1 * 2) = +1 + x, 1 + 1";
    ptoken = umn_expr_parse(&ptokens, &lexer);
    umn_ptoken_tree_print(&lexer, ptoken);
    umn_ptoken_strncpy(&lexer, ptoken, cbuffer, sizeof(cbuffer));
    puts(cbuffer);

    return 0;
}
