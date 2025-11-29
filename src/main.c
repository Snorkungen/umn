
#include <assert.h>
#define UMN_LEXER_IMPL
#include "./umn-lexer.h"
#include "umn-apa.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))

/* A new apa is required */

const size_t MARENA_ALLOCATION_SIZE = (1024 * 4); /* ~4kB largest possible allocation is that much*/

struct UMN_APA_Flag_Definition flag_defs[] = {
    {.short_name = 'd', .name = "decimal", .contiguous = 1},
    {.short_name = 'b', .name = "binary", .contiguous = 1},
    {.short_name = 'o', .name = "octal", .contiguous = 1},
    {.short_name = 'x', .name = "hex", .contiguous = 1},
    {.short_name = 'i', .name = "interctive", .contiguous = 0},
};

int dec2bin(char *dest, size_t dsize, int64_t v)
{
    int i = 0;
    dest[i++] = '0';
    dest[i++] = 'b';
    do
    {
        dest[i++] = ((v % 2) > 0) + '0';
        v /= 2;
    } while (v && i < dsize);

    return 0;
}

int main(int argc, char **argv)
{
    struct UMN_Arena *arenaptr = umn_arena_init(MARENA_ALLOCATION_SIZE);

    struct UMN_APA_Result apa_result = {
        .argc = argc,
        .argv = argv,
        .flagc = ARRAY_LEN(flag_defs),
        .flagd = flag_defs,
    };

    assert(umn_apa_parse(arenaptr, &apa_result) == 0);

    umn_Token token;
    umn_Lexer lexer = {};

    int flag_encoding_map[] = {10, 2, 8, 16};

    for (size_t flag_idx = 0; flag_idx < 4; flag_idx++)
    {
        int enocoding = flag_encoding_map[flag_idx];
        struct UMN_APA_Flag_Values *fv = apa_result.flags + flag_idx;
        if (fv->count == 0)
            continue;

        /* TODO: allow the program to do rudimentary calculations */

        for (size_t i = 0; i < fv->count; i++)
        {
            memset(&lexer, 0, sizeof(lexer));
            lexer.data = fv->values[i];
            lexer.data_len = strlen(lexer.data);

            printf("%2d: ", enocoding);
            int d = 0;
            while (umn_lexer_next(&lexer, &token), token.kind != UMN_KEOF)
            {
                if (token.kind & UMN_KERR && (token.kind & ~UMN_KERR) == UMN_KINTEGER)
                {
                    putchar('\n');
                    umn_token_print_error(&lexer, &token);
                    break;
                }

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

                if (d > 0)
                    printf(", ");

                static char input[100] = {};
                static char s[100] = {0};
                if (enocoding == 10)
                    snprintf(s, sizeof(s) - 1, "%lu", v);
                if (enocoding == 2)
                    dec2bin(s, sizeof(s) - 1, v);
                if (enocoding == 8)
                    snprintf(s, sizeof(s) - 1, "%lo", v);
                if (enocoding == 16)
                    snprintf(s, sizeof(s) - 1, "0x%lx", v);

                umn_token_strncpy(&lexer, &token, input, sizeof(input));

                if ((token.kind & UMN_KNUMERIC) == 0) {
                    printf("'%s'=%s", input, s);

                } else {
                    printf("%s=%s", input, s);
                }
                d++;
            }
            puts("");
        }
    }
    umn_arena_delete(arenaptr);
    return 0;
}