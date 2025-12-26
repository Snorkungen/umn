#define UMN_LEXER_IMPL

#include "umn/2expr.h"

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

int main(void)
{
    umn_PToken_Allocator ptokens = {0};
    umn_Lexer lexer = {
        .data = "2 + 4 + 8 * 6",
        .symbols = umn_expr_symbols,
    };

    // umn_PToken *ptoken;
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

    // lexer.position = 0;
    // lexer.data = " 0 + 1 * 2 * 4 + 8";
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

    // lexer.position = 0;
    // lexer.data = " (0) + 1 * 2 * 4 + 8";
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

    // lexer.position = 0;
    // lexer.data = " (0 + 1) * 2 * 4 + 8";
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

    // lexer.position = 0;
    // lexer.data = " (0 + 1) * 2 * (4 + 8)";
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

    // lexer.position = 0;
    // lexer.data = "(1)";
    // ptoken = umn_expr_parse(&ptokens, &lexer);
    // umn_ptoken_tree_print(&lexer, ptoken);

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

    char cbuffer[256] = {0};

    for (int i = 0; i < ARRAY_LEN(tests); i++)
    {
        /* soft reset the slab allocator */
        if (ptokens.items)
            umn_slab_drop(ptokens, ptokens.items->items);

        lexer.position = 0;
        lexer.data = tests[i].str;

        puts("---------------------------------------");
        puts(lexer.data);
        umn_PToken *res = umn_expr_parse(&ptokens, &lexer);

        int64_t value = compute(&lexer, res);

        printf("%s = %ld\n",
               umn_ptoken_strncpy(&lexer, res, cbuffer, sizeof(cbuffer)),
               compute(&lexer, res));

        if (value - tests[i].expected)
        {
            printf("\033[31m");
            umn_ptoken_tree_print(&lexer, res);
            printf("\033[0m");
            break;
        }
    }

    /* free all nodes  */
    umn_slab_free(ptokens, ptokens.items->items);

    return 0;
}