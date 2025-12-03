
#define UMN_LEXER_IMPL 1

#ifdef UMN_LEXER_IMPL
#include "./umn-lexer.h"
#endif

static umn_Symbol symbols[] = {
    {","},
    {"+", .attrs.data = 1},
    {"*", .attrs.data = 2},
};

int64_t calculate_compute(int64_t lvalue, int64_t rvalue, umn_Symbol_Attrs unit)
{
    switch (unit.data)
    {
    case 1:
        return lvalue + rvalue;
    case 2:
        return lvalue * rvalue;
    }
}

int64_t calculate(umn_Lexer *lexer, umn_Token *token)
{
    // first expect a number
    lexer->symbols = symbols;
    lexer->symbol_count = sizeof(symbols) / sizeof(symbols[0]);

    int state = 0;

    int depth = 0;
    int64_t lvalue[2], rvalue[2];
    umn_Symbol_Attrs unit[2];
    memset(unit, 0, sizeof(unit));

    while (umn_lexer_peek(lexer, token) == 0)
    {
        if (token->kind == UMN_KEOF)
        {
            depth--;
            break;
        }

        if (state == 0 || state == 2)
        {
            if (token->kind != UMN_KINTEGER)
            {
                token->kind |= UMN_KERR;
                break;
            }

            if (state == 2)
                rvalue[depth++] = umn_token_readi(lexer, token);
            else
                lvalue[depth = 0] = umn_token_readi(lexer, token);

            state = 1;
        }
        else if (state == 1)
        {
            if (token->kind != UMN_KSYMBOL || token->d.symbol.data == 0)
            {
                token->kind |= UMN_KERR;
                break;
            }

            if (depth > 0)
            {
                if (unit[depth - 1].data >= token->d.symbol.data)
                {
                    for (int i = 0; i < depth; i++)
                    {
                        lvalue[i + 1] = calculate_compute(lvalue[i], rvalue[i], unit[i]);
                    }
                    depth -= 1;
                }
                else
                {
                    /* TODO: set a flag to indicate that we should compute now the value whence we get an rvalue */
                }
            }

            memcpy(unit + depth, &token->d.symbol, sizeof(*unit));
            state = 2;
        }
        else
        {
            token->kind |= UMN_KERR;
            break;
        }

        umn_lexer_take(lexer, token);
    }

    for (int i = 0; i < depth; i++)
    {
        lvalue[i + 1] = calculate_compute(lvalue[i], rvalue[i], unit[i]);
    }

    return calculate_compute(lvalue[depth], rvalue[depth], unit[depth]);
}

int main(void)
{

    umn_Lexer lexer;
    umn_Token token;
    lexer.data = "10 + 2 * 3";
    lexer.data_len = strlen(lexer.data);

    printf("hello world v = %zu\n", calculate(&lexer, &token));

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