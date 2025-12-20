
#define UMN_LEXER_IMPL 1
#include "umn/lexer.h"
#include "umn/expr.h"

#define __MACRO__umn_lexer_init(__data__, __const_symbols__) {.data = __data__, .data_len = (sizeof(__data__) - 1), .symbols = __const_symbols__, .symbol_count = ARRAY_LEN(__const_symbols__)}
umn_expr_pnode_slab_t nodes = {0};

void run_expr_tests(void)
{
    umn_Lexer lexer;
    umn_PNode *res;

    const char *thing;
    const struct
    {
        const char *str;
        int64_t expected;
    } tests[] = {
        {"(2)", 2},
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
        /* soft reset the slab allocator */
        umn_slab_drop(nodes, nodes.items->items);
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
        res = umn_expr_parse(&nodes, &lexer);
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

    /* free all nodes  */
    umn_slab_free(nodes, nodes.items->items);
}

int compute_node(umn_Lexer *lexer, umn_PNode *node)
{
    uint64_t value = 0, lvalue, rvalue;

    struct
    {
        size_t count, capacity;
        umn_PNode *items[32];
    } stack = {.capacity = ARRAY_LEN(stack.items)};

    struct
    {
        size_t count, capacity;
        uint64_t items[64];
    } values = {.capacity = ARRAY_LEN(values.items)};
    struct
    {
        size_t count, capacity;
        unsigned items[64];
    } vstack = {.capacity = ARRAY_LEN(vstack.items)};

    umn_slice_push(stack, node);

    while (stack.count && (node = umn_slice_pop(stack)))
    {

        assert((node->token.kind & UMN_KERR) == 0);

        umn_pnode_print(lexer, node);

        if (node->token.kind == UMN_KINTEGER)
        { /* done happy */
            value = umn_token_readi(lexer, &node->token);
            umn_slice_push(values, value);
        }
        else if (node->token.kind & UMN_KBINOP && vstack.count > 0 && umn_slice_at(vstack, -1) == stack.count)
        {
            umn_slice_pop(vstack);

            rvalue = umn_slice_pop(values);
            lvalue = umn_slice_pop(values);

            if (NULL)
                ;
            else if (umn_token_issymbol(lexer, &node->token, "+"))
                value = lvalue + rvalue;
            else if (umn_token_issymbol(lexer, &node->token, "*"))
                value = lvalue * rvalue;

            umn_slice_push(values, value);
        }
        else if (node->token.kind & UMN_KBINOP && node->lvalue && node->rvalue)
        {
            umn_slice_push(vstack, stack.count);
            umn_slice_push(stack, node);
            umn_slice_push(stack, node->rvalue);
            umn_slice_push(stack, node->lvalue);
        }
        else
            UMN_TODO("handle differing values");
    }

    printf("value = %lu\n", umn_slice_at(values, -1));
}

int main(void)
{
    umn_PNode *node;
    umn_Token token;
    umn_Lexer lexer = __MACRO__umn_lexer_init("1 + 3 * 2", umn_expr_symbols);

    node = umn_expr_parse(&nodes, &lexer);

    compute_node(&lexer, node);

    /* run different tests and stuff */
    // run_expr_tests();

    umn_slab_free(nodes, nodes.items ? nodes.items->items : NULL);
    ;

    return 0;
}
