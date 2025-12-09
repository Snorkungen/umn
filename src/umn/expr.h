#ifndef UMN_EXPR_H
#define UMN_EXPR_H

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#include "./utils.h"
#include "./lexer.h"

static umn_Symbol umn_expr_symbols[] = {
    {","},
    {"("},
    {")"},
    {"+", .attrs.data = 1},
    {"*", .attrs.data = 2},
    {"**", .attrs.data = 3},
};

typedef struct umn_PNode
{
    umn_Token token;
    struct umn_PNode *lvalue; /* just use a bag of things */
    struct umn_PNode *rvalue;
} umn_PNode;

#define umn_pnode_prec(nodeptr) (nodeptr)->token.d.symbol.data

void umn_pnode_print_recurse(const umn_Lexer *lexer, umn_PNode *root)
{
    char buffer[1U << 7];

    if (root == NULL)
        return;

    umn_token_strncpy(lexer, &root->token, buffer, sizeof(buffer));

    if (root->lvalue == NULL)
    {
        printf("%s", buffer);
    }
    else
    {
        printf("(");
        umn_pnode_print_recurse(lexer, root->lvalue);
        printf(" %s ", buffer);
        umn_pnode_print_recurse(lexer, root->rvalue);
        printf(")");
    }
}
/* @see https://github.com/Snorkungen/expression/blob/master/interact.py#L423 */
void umn_pnode_print(const umn_Lexer *lexer, umn_PNode *root)
{

    // umn_pnode_print_recurse(lexer, root);
    // puts("");
    // return;
    // puts("Printing tree:");

    struct
    {
        size_t count, capacity;
        umn_PNode *items[16 << 2];
    } stack = {0};
    stack.count = 0, stack.capacity = ARRAY_LEN(stack.items);
    struct
    {
        size_t count, capacity;
        size_t items[ARRAY_LEN(stack.items) * 2];
    } stack_depth = {0};
    stack_depth.count = 0, stack_depth.capacity = ARRAY_LEN(stack_depth.items);

    size_t depth_map = 0;
    int depth;
    umn_PNode *curr = NULL;
    umn_slice_push(stack, root);
    umn_slice_push(stack_depth, 0);

    while (stack.count > 0 && (curr = umn_slice_pop(stack)) != NULL)
    {
        depth_map = umn_slice_pop(stack_depth);
        depth = 0;

        while ((depth_map >> depth) > 0)
            depth += 1;

        for (int i = 0; i < (depth - 1); i++)
        {
            if (depth_map & (1 << i))
                printf("│  ");
            else
                printf("   ");
        }

        if (depth > 0 && (stack_depth.count == 0 || stack_depth.items[stack_depth.count - 1] < depth_map))
            printf("└──");
        else if (depth > 0)
            printf("├──");

        umn_token_print(lexer, &curr->token);

        if (depth >= 1 && (stack_depth.items[stack_depth.count - 1] & (1 << (depth - 1))) == 0)
            depth_map ^= 1UL << (depth - 1);

        // depth_map = (depth_map | (1 << (depth))) ^ (1UL << (depth > 0 ? depth - 1 : 0));
        if (curr->rvalue)
        {
            umn_slice_push(stack, curr->rvalue);
            umn_slice_push(stack_depth, depth_map | (1 << (depth)));
        }

        if (curr->lvalue)
        {
            umn_slice_push(stack, curr->lvalue);
            umn_slice_push(stack_depth, depth_map | (1 << (depth)));
        }
    }
}

uint64_t umn_pnode_compute(const umn_Lexer *lexer, umn_PNode *root)
{
    assert(root != NULL);

    if (root->token.kind == UMN_KINTEGER)
    {
        return umn_token_readi(lexer, &root->token);
    }
    uint64_t res, lv = umn_pnode_compute(lexer, root->lvalue), rv = umn_pnode_compute(lexer, root->rvalue);

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

umn_PNode *umn_expr_parse__set_value(umn_PNode *dest_node, umn_PNode *value)
{
    if (dest_node->lvalue == NULL)
        dest_node->lvalue = value;
    else if (dest_node->rvalue == NULL)
        dest_node->rvalue = value;
    else
        return NULL;
    return value;
}

int umn_expr_parse(const char *inp)
{
    puts(inp);

    umn_Token token;
    umn_Lexer lexer = {
        .data = inp,
        .data_len = strlen(inp),
        .symbols = umn_expr_symbols,
        .symbol_count = ARRAY_LEN(umn_expr_symbols),
    };

    UMN_SLAB_T(umn_PNode)
    nodes = {0};

    struct
    {
        size_t count, capacity;
        umn_PNode *items[64]; /* just by default allow for so many because reasons */
    } stack;
    stack.count = 0, stack.capacity = ARRAY_LEN(stack.items); /* init stack */

    /* store the bracket  locations */
    struct
    {
        size_t count, capacity;
        int items[ARRAY_LEN(stack.items) >> 2];
    } bracket_stack;
    bracket_stack.count = 0, bracket_stack.capacity = ARRAY_LEN(stack.items); /* init stack */

    /* initialize the state */
    bool expect_value = true;
    /* push default values */
    umn_slice_push(stack, umn_slab_alloc(nodes));
    umn_slice_push(bracket_stack, stack.count);

    while (umn_lexer_next(&lexer, &token) == 0)
    {
        if (token.kind == UMN_KEOF || (token.kind & UMN_KERR))
            break;

        if (expect_value && umn_token_issymbol(&lexer, &token, "("))
        { /* push an empty node onto the stack */
            umn_slice_push(stack, umn_slab_alloc(nodes));
            umn_slice_push(bracket_stack, stack.count);
            continue;
        }
        else if (!expect_value && umn_token_issymbol(&lexer, &token, ")"))
        {
            if (bracket_stack.count <= 1)
            {
                token.kind |= UMN_KERR;
                break;
            }

            /* reconcile */
            int base_count = umn_slice_pop(bracket_stack);
            umn_PNode *tmp = umn_slice_at(stack, base_count - 1);

            for (int i = base_count - 1; i < stack.count - 1 && umn_slice_at(stack, i)->token.kind; i++)
            {
                assert(umn_pnode_prec(umn_slice_at(stack, i)) < umn_pnode_prec(umn_slice_at(stack, i + 1)));
                assert(umn_slice_at(stack, i)->rvalue == NULL);
                umn_slice_at(stack, i)->rvalue = umn_slice_at(stack, i + 1);
            }

            if (umn_slice_at(stack, base_count - 1)->token.kind == 0 &&
                (base_count - 1) == umn_slice_at(bracket_stack, -1))
            {
                /* promote lvalue to the token value */
                assert(umn_slice_at(stack, base_count - 1)->rvalue == NULL);
                memcpy(&umn_slice_at(stack, base_count - 1)->token,
                       &umn_slice_at(stack, base_count - 1)->lvalue->token, sizeof(token));
                umn_slice_at(stack, base_count - 1)->lvalue = NULL;

                if (nodes.count && umn_slice_at(nodes, -1).count)
                {
                    umn_slice_pop(umn_slice_at(nodes, -1));
                    if (umn_slice_at(nodes, -1).count == 0)
                        umn_slice_pop(nodes);
                }
            }

            stack.count = base_count - 1;
            assert(umn_expr_parse__set_value(umn_slice_at(stack, -1), tmp) != NULL);
            continue;
        }
        else if (expect_value)
        {
            if (token.kind != UMN_KINTEGER)
            {
                token.kind |= UMN_KERR;
                break;
            }

            umn_PNode *tmp = umn_expr_parse__set_value(
                umn_slice_at(stack, -1),
                umn_slab_alloc(nodes));
            assert(tmp != NULL);
            memcpy(&tmp->token, &token, sizeof(token));
        }
        else if (!expect_value && umn_slice_at(stack, -1)->token.kind == 0)
        {
            memcpy(&umn_slice_at(stack, -1)->token, &token, sizeof(token));
        }
        else if (!expect_value)
        {
            int base_count = umn_slice_at(bracket_stack, -1);

            umn_slice_push(stack, umn_slab_alloc(nodes));
            memcpy(&umn_slice_at(stack, -1)->token, &token, sizeof(token));

            if (stack.count > (base_count) && umn_pnode_prec(umn_slice_at(stack, -2)) < umn_pnode_prec(umn_slice_at(stack, -1)))
            { /* higher prec */ /* steal the previous rvalue i.e [(2 + 3) * ] -> [2 + (3 * ) ] */
                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2)->rvalue;
                umn_slice_at(stack, -2)->rvalue = NULL;
            }
            else if (stack.count > (base_count) && umn_pnode_prec(umn_slice_at(stack, -2)) == umn_pnode_prec(umn_slice_at(stack, -1)))
            { /* set the lvalue */
                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2);
                umn_slice_at(stack, -2) = umn_slice_at(stack, -1);
                umn_slice_pop(stack);
            }
            else if (stack.count > (base_count) && umn_pnode_prec(umn_slice_at(stack, -2)) > umn_pnode_prec(umn_slice_at(stack, -1)))
            { /* equal or lower prec */
                for (int i = stack.count - 2; i >= base_count; i--)
                {
                    assert(umn_pnode_prec(umn_slice_at(stack, i - 1)) <= umn_pnode_prec(umn_slice_at(stack, i)));
                    assert(umn_slice_at(stack, i - 1)->rvalue == NULL);
                    umn_slice_at(stack, i - 1)->rvalue = umn_slice_at(stack, i);
                }

                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, base_count - 1);
                umn_slice_at(stack, base_count - 1) = umn_slice_at(stack, -1);
                stack.count = base_count;
            }
            else
            {
                UMN_TODO("HANDLE THIS CASE");
            }
        }

        expect_value = !expect_value;
    }

    assert((token.kind & UMN_KERR) == 0);
    assert(bracket_stack.count == 1 && umn_slice_at(bracket_stack, 0) == 1);

    /* a better way would be a two step approach decrement until hits something with a higher prec  */

    if (stack.count > 1)
    {
        for (int i = stack.count - 1; i > 0; i--)
        {
            assert(umn_pnode_prec(umn_slice_at(stack, i - 1)) <= umn_pnode_prec(umn_slice_at(stack, i)));
            assert(umn_slice_at(stack, i - 1)->rvalue == NULL);
            umn_slice_at(stack, i - 1)->rvalue = umn_slice_at(stack, i);
        }

        stack.count = 1;
    }

    // umn_pnode_print(&lexer, umn_slice_at(stack, 0));
    umn_pnode_print_recurse(&lexer, umn_slice_at(stack, 0));
    printf(" = %zu\n", umn_pnode_compute(&lexer, umn_slice_at(stack, 0)));

    return 0;
}

#endif