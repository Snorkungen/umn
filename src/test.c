#define UMN_LEXER_IMPLEMENTATION
#define UMN_UTILS_IMPLEMENTATION
#define UMN_EXPR_IMPLEMENTATION

#include "umn/expr.h"

static unsigned char global_buffer[256];

int main(void)
{
    for (int i = 0; i < sizeof(global_buffer); i++)
    {
        global_buffer[i] = i & 1 ? 0xff : 0xaa;
    }

    umn_meminit(global_buffer, sizeof(global_buffer));

    umn_memfree(umn_memalloc(16));
    umn_memalloc(16);
    umn_memfree(umn_memalloc(16));
    umn_memfree(umn_memalloc(16));

    return 0;
}