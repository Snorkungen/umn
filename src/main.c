#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "umn.h"
#include "umn-apa.h"

const size_t MARENA_ALLOCATION_SIZE = (1024 * 4); /* ~4kB largest possible allocation is that much*/

int main(int argc, char **argv)
{
    struct UMN_Arena *arenaptr = umn_arena_init(MARENA_ALLOCATION_SIZE);

    struct UMN_APA_Flag_Definition flag_defs[] = {
        {.short_name = 'd', .name = "decimal", .contiguous = 1},
        {.short_name = 'b', .name = "binary", .contiguous = 1},
        {.short_name = 'o', .name = "octal", .contiguous = 1},
        {.short_name = 'x', .name = "hex", .contiguous = 1},
        {.short_name = 'i', .name = "interctive", .contiguous = 0},
    };

    struct UMN_APA_Result apa_result = {
        .argc = argc,
        .argv = argv,
        .flagc = 5,
        .flagd = flag_defs,
    };

    if (umn_apa_parse(arenaptr, &apa_result))
    {
        assert(0); /* the parser should not fail ... */
    }

    if (apa_result.keyc > 1)
    {
        puts("Keys:");
        for (size_t i = 0; i < apa_result.keyc; i++)
        {
            puts(*(apa_result.keys + i));
        }
    }

    struct UMN_Tokeniser tokeniser = {
        .keywords = UMN_keywords,
        .keywords_count = UMN_KWORD_COUNT,
    };

    int flag_encoding_map[] = {10, 2, 8, 16};

    for (size_t flag_idx = 0; flag_idx < 4; flag_idx++)
    {
        int enocoding = flag_encoding_map[flag_idx];
        struct UMN_APA_Flag_Values *fv = apa_result.flags + flag_idx;
        if (fv->count == 0)
        {
            continue;
        }

        /* you use the tokeniser for now */
        /* TODO: allow the program to do rudimentary calculations */

        for (size_t i = 0; i < fv->count; i++)
        {
            tokeniser.data = fv->values[i];
            tokeniser.data_length = strlen(tokeniser.data);

            struct UMN_Token token = umn_tokeniser_get(&tokeniser);
            while (!umn_kind_compare(token.kind, UMN_KIND_EOF) && !umn_kind_compare(token.kind, UMN_KIND_ERROR))
            {
                if (!umn_kind_compare(token.kind, UMN_KIND_INTEGER))
                {
                    /* ignore token */;
                }
                else
                {
                    char s[100] = {0};
                    umn_token_to_string(&token, s, 99);
                    printf("%s = ", s);

                    token.value[1] = enocoding;

                    umn_token_to_string(&token, s, 99);
                    printf("%s\n", s);
                }

                token = umn_tokeniser_get(&tokeniser);
            }
        }
    }

    umn_arena_delete(arenaptr);
    return 0;
}
