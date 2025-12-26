
#include "umn/perfm.h"

// #define MANUAL_INLINE
#define UMN_LEXER_IMPL

// #define TEST_2

#ifdef TEST_2
#include "umn/2lexer.h"
#else
#include "umn/lexer.h"
#endif

static umn_Symbol symbols[] = {
    {"+"},
    {"*"},
    {"-"},
};

size_t start, end, sum;

static umn_Lexer init_lexer(const char *data);

static inline umn_Lexer init_lexer(const char *data)
{
#ifndef TEST_2
    return (umn_Lexer){
        .data = data,
        .data_len = strlen(data),
        .symbol_count = ARRAY_LEN(symbols),
        .symbols = symbols,
    };
#else
    return (umn_Lexer){
        .data = data,
        .symbols = {
            .count = ARRAY_LEN(symbols),
            .items = symbols,
        },
    };

#endif
}

#define ITER_COUNT (4 * 10E3)

int main(void)
{
    umn_perfm_t *inner;

    inner = umn_perfm_create_new(NULL, "lexer");


    umn_Token token;
    // umn_Lexer lexer = init_lexer("1 0.0 0a b 'hello' 0x254 0b11153 0654");
    // umn_Lexer lexer = init_lexer("1 0.0 0a b 'hello' 0x254 0b11153 0654 20032");
    umn_Lexer lexer = init_lexer("fdslakjfldksja-jdlksafjd-sa-fdsaf-d-sfaj-s'fdjsafds'fdsaf'dsaf'dsa'f'dsaf'dsklfjdsklafd'");

    for (unsigned i = 0; i < ITER_COUNT; i++)
    {
        lexer.position = 0;

        do
        {
            umn_perfm_open_for(inner)
            {
                umn_lexer_next(&lexer, &token);
            }
        } while ((token.kind & UMN_KERR) == 0 && token.kind != UMN_KEOF);

        if ((i % (size_t)(ITER_COUNT / 4)) == 0)
        {
            // umn_perfm_report(lexer_pt);
            umn_perfm_report(inner);
        }
    }

    return 0;
    lexer.position = 0;

    unsigned long long start, stop;
    do
    {
        umn_lexer_next(&lexer, &token);
        umn_token_print(&lexer, &token);
    } while ((token.kind & UMN_KERR) == 0 && token.kind != UMN_KEOF);

    return 0;
}
