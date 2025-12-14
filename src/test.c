
#define UMN_LEXER_IMPL 1
#include "umn/lexer.h"
#include "umn/expr.h"

#define __MACRO__umn_lexer_init(__data__, __const_symbols__) {.data = __data__, .data_len = (sizeof(__data__) - 1), .symbols = __const_symbols__, .symbol_count = ARRAY_LEN(__const_symbols__)}

int main(void)
{
    /* LILVODKA REQUIRES,  -I=core sqrt(x), pow(b, e), ln(x), .... */

    umn_expr_pnode_slab_t nodes = {0};
    umn_Lexer lexer = __MACRO__umn_lexer_init("f(a, b, c) = (x + y) * (x + y)", umn_expr_symbols); /* recursion is not allowed */

    umn_PNode *res = umn_expr_parse_func(&lexer, &nodes);
    if (res->token.kind & UMN_KERR)
    {
        umn_token_print_error(&lexer, &res->token);
        puts("failed ...");
        abort();
    }

    /*
        Parse pseudo-code

        parse a statement
        do:
            Parse_Expr:
                Parse_value
        while( next token is ",")

        Parse_Value(node):
            if Parse_Numeric(node) return
            if Parse_Custom(node) return
            if Parse_Func_Def(node) return

    */

    const char *thing;
    const struct
    {
        const char *str;
        int64_t expected;
    } tests[] = {
        {"(1) + (2)", 3},
        {"(1) + (2 * 3 + 1) ** 4 + 5", 7 * 7 * 7 * 7 + 6},
        {"1 + 2 ** 2 ", 5},
        {"1 + 2 * 10 * 3 ** 4 + 5", 1626},
        {"(1) + (2 * 3 + 1) ** 4 + 5", 2407},
        {"1 + (2) * 3 * 4", 25},
        {"(1) + (2 * 3)", 7},
        {"1 + 2 + (3 * 4) + 5 * 6", 45},
        {"1 + 2 * (3 + 4) * 5", 71},
        {"(1 * 1) + 2 + 3 +4 + 5+ 6 * 1", 21},
    };

    for (int i = 0; i < ARRAY_LEN(tests); i++)
    {
        thing = tests[i].str;

        {
            memset(&lexer, 0, sizeof(lexer));
            lexer.symbol_count = ARRAY_LEN(umn_expr_symbols);
            lexer.symbols = umn_expr_symbols;
            lexer.data = thing;
            lexer.data_len = strlen(thing);
        }

        puts("---------------------------------------");
        puts(lexer.data);
        res = umn_expr_parse_actual_thing_that_does_stuff(&nodes, &lexer);
        int64_t value = umn_pnode_compute(&lexer, res);
        umn_pnode_print_recurse(&lexer, res);
        printf(" = %ld\n", umn_pnode_compute(&lexer, res));

        if (value - tests[i].expected)
        {
            printf("\033[31m");
            umn_pnode_print(&lexer, res);
            printf("\033[0m");
            break;
        }
    }

    return 0;
}