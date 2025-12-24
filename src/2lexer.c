
#include "umn/perfm.h"

// #define MANUAL_INLINE
#define UMN_LEXER_IMPL

#define TEST_2

#ifdef TEST_2
#include "umn/3lexer.h"
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

#define ITER_COUNT (4 * 2 * 1e4)

int main(void)
{
    umn_perfm_t *outer = umn_perfm_create(), *inner = umn_perfm_create();
    lexer_pt = umn_perfm_create();

    umn_Token token;
    // umn_Lexer lexer = init_lexer("1 0.0 0a b 'hello' 0x254 0b11153 0654");
    // umn_Lexer lexer = init_lexer("1 0.0 0a b 'hello' 0x254 0b11153 0654 20032");
    umn_Lexer lexer = init_lexer("fdslakjfldksja-jdlksafjd-sa-fdsaf-d-sfaj-s'fdjsafds'fdsaf'dsaf'dsa'f'dsaf'dsklfjdsklafd'");

    for (unsigned i = 0; i < ITER_COUNT; i++)
    {
        lexer.position = 0;

        umn_perfm_open_stop(outer)
        {
            do
            {
                umn_perfm_open_stop(inner)
                {
                    umn_lexer_next(&lexer, &token);
                }
            } while ((token.kind & UMN_KERR) == 0 && token.kind != UMN_KEOF);
        }

        if ((outer->n % (size_t)(ITER_COUNT / 4)) == 0)
        {
            printf("outer = %llu, inner = %llu, lexer_inner = %llu\n", outer->mavg, inner->mavg,  lexer_pt->mavg);

            umn_perfm_reset(outer);
            umn_perfm_reset(inner);
            umn_perfm_reset(lexer_pt);
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
