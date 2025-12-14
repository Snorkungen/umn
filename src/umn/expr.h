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

typedef UMN_SLAB_T(umn_PNode) umn_expr_pnode_slab_t;

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

int64_t umn_pnode_compute(const umn_Lexer *lexer, umn_PNode *root)
{
    assert(root != NULL);

    if (root->token.kind == UMN_KINTEGER)
    {
        return umn_token_readi(lexer, &root->token);
    }
    int64_t res, lv = umn_pnode_compute(lexer, root->lvalue), rv = umn_pnode_compute(lexer, root->rvalue);

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
    if (value == NULL)
        return NULL;

    if (dest_node->lvalue == NULL)
        dest_node->lvalue = value;
    else if (dest_node->rvalue == NULL)
        dest_node->rvalue = value;
    else
        return NULL;

    return value;
}

umn_PNode *umn_expr_parse_actual_thing_that_does_stuff(umn_expr_pnode_slab_t *nodes, umn_Lexer *lexer);
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

    umn_expr_pnode_slab_t nodes = {0};

    umn_PNode *res = umn_expr_parse_actual_thing_that_does_stuff(&nodes, &lexer);
    umn_pnode_print(&lexer, res);

    umn_pnode_print_recurse(&lexer, res);
    printf(" = %ld\n", umn_pnode_compute(&lexer, res));
}

/* NOTE: this function will crash the program rather than return an invalid node */
umn_PNode *umn_pnode_alloc(umn_expr_pnode_slab_t *nodes, const umn_Token *token)
{
    umn_PNode *res = umn_slab_alloc((*nodes));

    if (res == NULL)
        UMN_TODO("malloc failed");

    if (token)
        memcpy(&res->token, token, sizeof(*token));

    return res;
}

int umn_pnode_free_last_allocated_node(umn_expr_pnode_slab_t *nodes)
{
    if (nodes->count == 0 || umn_slice_at((*nodes), -1).count == 0)
        return -1;

    umn_slice_pop(
        umn_slice_at((*nodes), -1));

    if (umn_slice_at((*nodes), -1).count == 0)
        umn_slice_pop((*nodes));

    return 0;
}

umn_PNode *umn_expr_parse_actual_thing_that_does_stuff(umn_expr_pnode_slab_t *nodes, umn_Lexer *lexer)
{
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

    umn_slice_push(stack, umn_pnode_alloc(nodes, NULL));
    umn_slice_push(bracket_stack, stack.count);

    umn_Token token;
    while (umn_lexer_next(lexer, &token) == 0)
    {
        if (token.kind == UMN_KEOF || (token.kind & UMN_KERR))
            break;

        if (expect_value && umn_token_issymbol(lexer, &token, "("))
        { /* push an empty node onto the stack */
            umn_slice_push(stack, umn_pnode_alloc(nodes, NULL));
            umn_slice_push(bracket_stack, stack.count);
            continue;
        }
        else if (!expect_value && umn_token_issymbol(lexer, &token, ")"))
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

            if (tmp->token.kind == 0)
            {
                assert(tmp->rvalue == NULL && tmp->lvalue);
                memcpy(tmp, tmp->lvalue, sizeof(*tmp));
                umn_pnode_free_last_allocated_node(nodes);
            }

            stack.count = base_count - 1;
            if (umn_expr_parse__set_value(umn_slice_at(stack, -1), tmp) == NULL)
                UMN_TODO("HANDLE: errors");

            continue;
        }
        else if (expect_value)
        {
            umn_PNode *value = NULL;
            if (token.kind == UMN_KINTEGER)
                value = umn_pnode_alloc(nodes, &token);

            if (umn_expr_parse__set_value(umn_slice_at(stack, -1), umn_pnode_alloc(nodes, &token)) == NULL)
                UMN_TODO("HANDLE: errors");
        }
        else if (!expect_value && umn_slice_at(stack, -1)->token.kind == 0)
        {
            memcpy(&umn_slice_at(stack, -1)->token, &token, sizeof(token));
        }
        else if (!expect_value)
        {
            int base_count = umn_slice_at(bracket_stack, -1);

            umn_slice_push(stack, umn_pnode_alloc(nodes, &token));

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

    if (token.kind & UMN_KERR)
        UMN_TODO("HANDLE: errors");

    assert(bracket_stack.count == 1 && umn_slice_at(bracket_stack, 0) == 1);

    if (umn_slice_at(stack, 0)->token.kind == 0)
    {
        assert(umn_slice_at(stack, 0)->lvalue);
        memcpy(umn_slice_at(stack, 0), umn_slice_at(stack, 0)->lvalue, sizeof(umn_PNode));

        /* leaking a node can't be bothered because this is a bounded bounded */
    }
    else if (stack.count > 1)
    {
        for (int i = stack.count - 1; i > 0; i--)
        {
            assert(umn_pnode_prec(umn_slice_at(stack, i - 1)) <= umn_pnode_prec(umn_slice_at(stack, i)));
            assert(umn_slice_at(stack, i - 1)->rvalue == NULL);
            umn_slice_at(stack, i - 1)->rvalue = umn_slice_at(stack, i);
        }

        stack.count = 1;
    }

    return umn_slice_at(stack, 0);
}

/* return the resultant node into the dest pointer thingy .. */
umn_PNode *umn_expr_parse_func(umn_Lexer *lexer, umn_expr_pnode_slab_t *nodes)
{
    umn_PNode *node = umn_slab_alloc(*nodes);
    umn_Token token;

    assert(node != NULL);

    if (umn_lexer_peek(lexer, &node->token)) /* non zeror means error */
        return node;                         /* how to do errors */

    if (node->token.kind != UMN_KLITERAL)
    {
        node->token.kind |= UMN_KERR;
        return node;
    }

    /* commit no going back ... */
    umn_lexer_take(lexer, &node->token);

    /* the next token must be a opening bracket ..*/
    if (umn_lexer_peek(lexer, &token) || !umn_token_issymbol(lexer, &token, "("))
    {
        token.kind |= UMN_KERR;
        memcpy(&node->token, &token, sizeof(token));
        return node;
    }

    /* commit no going back ... */
    umn_lexer_take(lexer, &token);

    while (umn_lexer_next(lexer, &token) == 0)
    {
        if (umn_token_issymbol(lexer, &token, ")"))
            break;

        if (token.kind != UMN_KLITERAL || token.kind == UMN_KEOF)
        {
            token.kind |= UMN_KERR;
            memcpy(&node->token, &token, sizeof(token));
            return node;
        }

        /* do the magic thing that parses a valid value */
        /* and then add this to some local stack so that the thing can commit the thing ... */

        umn_token_print(lexer, &token);

        umn_lexer_peek(lexer, &token);
        if (umn_token_issymbol(lexer, &token, ","))
        {
            umn_lexer_take(lexer, &token);
            continue;
        }
        else if (umn_token_issymbol(lexer, &token, ")"))
        {
            umn_lexer_take(lexer, &token);
            break;
        }
        else
        {
            token.kind |= UMN_KERR;
            memcpy(&node->token, &token, sizeof(token));
            return node;
        }
    }

    /* I think function definition is something like */
    /*
                =
               / \
              /   \
          f(x)     (x - 1)
        x -> NULL


    */

    /* where f is something */
    /* rvalue => umn_PNode ** an array of pointers so you can just memcpy from the stack or a linked list ...  */

    return node;
}

#endif