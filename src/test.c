
#define UMN_LEXER_IMPL 1

#ifdef UMN_LEXER_IMPL
#include "./umn/lexer.h"
#endif

#include "umn/expr.h"

int main(void)
{

    umn_expr_parse("1 + 2 ** 2 ");
    puts("---------------------------------------");
    umn_expr_parse("1 + 2 * 10 * 3 ** 4 + 5");
    puts("---------------------------------------");
    umn_expr_parse("(1) + (2 * 3 + 1) ** 4 + 5");
    puts("---------------------------------------");
    umn_expr_parse("1 + (2) * 3 * 4");
    puts("---------------------------------------");
    umn_expr_parse("(1) + (2 * 3)");
    puts("---------------------------------------");
    umn_expr_parse("1 + 2 + (3 * 4) + 5 * 6");
    puts("---------------------------------------");
    umn_expr_parse("1 + 2 * (3 + 4) * 5");
    puts("---------------------------------------");
    umn_expr_parse("(1 * 1) + 2 + 3 +4 + 5+ 6 * 1");
    puts("---------------------------------------");
    // umn_expr_parse("1 + 2 * (3 + 4) * 5");
    puts("---------------------------------------");

    return 0;

#ifdef UMN_LEXER_IMPL_
    umn_Symbol symbols[] = {
        {","},
        {"a", .attrs.flags = 1},
    };

    umn_Lexer lexer = {.data = "10.0 aa,,03 '\\'' 0ba11 0xaafa 1.1E3", .symbols = symbols, .symbol_count = sizeof(symbols) / sizeof(symbols[0])};
    lexer.data_len = strlen(lexer.data);

    umn_Token token;

    do
    {
        umn_lexer_next(&lexer, &token);
        if (token.kind & UMN_KERR)
        {
            umn_token_print_error(&lexer, &token);
        }

        if (token.kind == UMN_KINTEGER)
            printf("v = %ld ", umn_token_readi(&lexer, &token));
        if (token.kind == UMN_KFRACTION)
            printf("v = %g ", umn_token_readf(&lexer, &token));
        if (token.kind == UMN_KSYMBOL)
            printf("v = %d ", umn_token_issymbol(&lexer, &token, ","));
        if (token.kind & UMN_KLITERAL)
            printf("v = %d ", umn_token_litcmp(&lexer, &token, ","));

        umn_token_print(&lexer, &token);

    } while (token.kind);
#endif

    return 1;
}