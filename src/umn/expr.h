#ifndef UMN_EXPR_H
#define UMN_EXPR_H

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#include "./utils.h"
#include "./lexer.h"

static const umn_Token_Kind UMN_KBINOP = UMN_K__1,
                            UMN_KFNCDEF = UMN_K__2;

static umn_Symbol umn_expr_symbols[] = {
    {","},
    {"("},
    {")"},
    {"=", .attrs.data = 1},
    {"+", .attrs.data = 2},
    {"*", .attrs.data = 3},
    {"**", .attrs.data = 4},
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

    if (root->token.kind & UMN_KBINOP)
    {
        printf("(");
        umn_pnode_print_recurse(lexer, root->lvalue);
        printf(" %s ", buffer);
        umn_pnode_print_recurse(lexer, root->rvalue);
        printf(")");
    }
    else if (root->token.kind & UMN_KFNCDEF)
    {
        umn_token_strncpy(lexer, &root->token, buffer, sizeof(buffer));
        printf("%s(", buffer);

        for (umn_PNode *child = root->lvalue; child != NULL; child = child->rvalue)
        {
            umn_token_strncpy(lexer, &child->token, buffer, sizeof(buffer));

            if (child->rvalue)
                printf("%s, ", buffer);
            else
                printf("%s", buffer);
        }
        putchar(')');
    }
    else
        printf("%s", buffer);
}
/* @see https://github.com/Snorkungen/expression/blob/master/interact.py#L423 */
void umn_pnode_print(const umn_Lexer *lexer, umn_PNode *root)
{
    typedef struct
    {
        umn_PNode *node;
        size_t depth_map;
    } stack_item_t;
    struct
    {
        size_t count, capacity;
        stack_item_t items[64];
    } stack_novo = {.capacity = ARRAY_LEN(stack_novo.items)};

    size_t depth_map = 0;
    int depth;
    umn_PNode *curr = NULL;

    /* init stack */
    umn_slice_push(stack_novo, ((stack_item_t){root}));
    while (stack_novo.count > 0)
    {

        curr = umn_slice_at(stack_novo, -1).node;
        depth_map = umn_slice_at(stack_novo, -1).depth_map;
        stack_novo.count--;

        assert(curr);

        /* get the position of the largest set bit */
        depth = 0;
        while ((depth_map >> depth))
            depth++;

        for (int i = 0; i < (depth - 1); i++)
        {
            if (depth_map & (1 << i))
                printf("│  ");
            else
                printf("   ");
        }

        if (depth > 0 && (umn_slice_at(stack_novo, -1).depth_map < depth_map || stack_novo.count == 0))
            printf("└──");
        else if (depth > 0)
            printf("├──");

        umn_token_print(lexer, &curr->token);

        if (depth >= 1 && ((umn_slice_at(stack_novo, -1).depth_map & (1 << (depth - 1))) == 0))
            depth_map ^= 1UL << (depth - 1);

        depth_map |= 1 << depth;

        if (curr->token.kind & UMN_KBINOP)
        {
            umn_slice_push(stack_novo, ((stack_item_t){curr->rvalue, depth_map}));
            umn_slice_push(stack_novo, ((stack_item_t){curr->lvalue, depth_map}));
        }
        else if (curr->token.kind & UMN_KFNCDEF)
        {
            for (umn_PNode *child = curr->rvalue; child != NULL; child = child->lvalue)
                umn_slice_push(stack_novo, ((stack_item_t){child, depth_map}));
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

    assert(root->token.kind & UMN_KBINOP);

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

umn_PNode *umn_expr_parse_value_fncddef(umn_expr_pnode_slab_t *nodes, umn_Lexer *lexer, umn_Token *token);

umn_PNode *umn_expr_parse(umn_expr_pnode_slab_t *nodes, umn_Lexer *lexer)
{
    umn_Token token;

    /* NOTE: this is avoiding a specific bug */
    if (umn_lexer_peek(lexer, &token) == 0 && token.kind == UMN_KEOF)
        return NULL;

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
        int items[ARRAY_LEN(stack.items) / 2];
    } bracket_stack;
    bracket_stack.count = 0, bracket_stack.capacity = ARRAY_LEN(stack.items); /* init stack */

    /* initialize the state */
    bool expect_value = true;
    /* push default values */

    umn_slice_push(stack, umn_pnode_alloc(nodes, NULL));
    umn_slice_push(bracket_stack, stack.count);

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
            if (token.kind == UMN_KLITERAL)
            {
                value = umn_expr_parse_value_fncddef(nodes, lexer, &token);
            }

            if (umn_expr_parse__set_value(umn_slice_at(stack, -1), value) == NULL)
                UMN_TODO("HANDLE: errors");
        }
        else if (!expect_value && umn_token_issymbol(lexer, &token, ","))
        {
            umn_lexer_give(lexer, &token);
            break;
        }
        else if (!expect_value && (token.kind != UMN_KSYMBOL || token.d.symbol.data == 0 /* NOTE: this should probably be a flag */))
        {
            token.kind |= UMN_KERR;
            memcpy(&umn_slice_at(stack, -1)->token, &token, sizeof(token));
            return umn_slice_at(stack, -1);
        }
        else if (!expect_value && umn_slice_at(stack, -1)->token.kind == 0)
        {
            token.kind |= UMN_KBINOP;
            memcpy(&umn_slice_at(stack, -1)->token, &token, sizeof(token));
        }
        else if (!expect_value)
        {
            int base_count = umn_slice_at(bracket_stack, -1);

            token.kind |= UMN_KBINOP;
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

    if (expect_value)
    {
        umn_slice_at(stack, 0)->token.kind |= UMN_KERR;
    }

    return umn_slice_at(stack, 0);
}

/* return the resultant node into the dest pointer thingy .. */

umn_PNode *umn_expr_parse_value_fncddef(umn_expr_pnode_slab_t *nodes, umn_Lexer *lexer, umn_Token *token)
{
    if (token->kind != UMN_KLITERAL)
        return NULL;

    umn_PNode *node = umn_pnode_alloc(nodes, token);

    /*  parse the following (a, b, c) */
    if (umn_lexer_peek(lexer, token))
    {
        token->kind |= UMN_KERR;
        return NULL;
    }
    else if (!umn_token_issymbol(lexer, token, "("))
    {
        return node;
    }

    umn_lexer_take(lexer, token);

    node->token.kind |= UMN_KFNCDEF; /* this means a fncdef for some reason */

    /* linked list of things */
    bool expect_value = true;
    while (umn_lexer_next(lexer, token) == 0 && token->kind != UMN_KEOF)
    {
        if (umn_token_issymbol(lexer, token, ")"))
            return node; /* this might be confusing but  aghhh */

        if (!expect_value && umn_token_issymbol(lexer, token, ","))
        {
            expect_value = true;
            continue;
        }

        /* error */
        if (!expect_value || token->kind != UMN_KLITERAL)
            break;

        umn_PNode *tmp = umn_pnode_alloc(nodes, token);
        if (node->lvalue)
        {
            tmp->lvalue = node->rvalue;
            node->rvalue->rvalue = tmp;
            node->rvalue = tmp;
        }
        else
        {
            node->lvalue = node->rvalue = tmp;
        }

        expect_value = false;
    }

    token->kind |= UMN_KERR;
    return NULL;
}

#endif
