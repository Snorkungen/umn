#ifndef UMN_EXPR_H
#define UMN_EXPR_H

#include "2lexer.h"

static const umn_Token_Kind UMN_KBINOP = UMN_K__1,
                            UMN_KBRACK = UMN_K__2,
                            UMN_KFNCDEF = UMN_K__3;

static const uint32_t UMN_SF_BINOP = 0x80;

static const umn_Symbol umn_expr_symbols__[] = {
    /* minimum set of symbols required ... */
    {","},
    {"("},
    {")"},
    {"=", .flags = UMN_SF_BINOP, .data = 16},
    {"+", .flags = UMN_SF_BINOP, .data = 18},
    {"*", .flags = UMN_SF_BINOP, .data = 19},
    {"**", .flags = UMN_SF_BINOP, .data = 20},
};
static const umn_Lexer_Symbols umn_expr_symbols = {
    .count = ARRAY_LEN(umn_expr_symbols__), /* manually counting is annoying */
    .items = umn_expr_symbols__,
};

/* parse token is an extension of a token with */
typedef struct umn_PToken
{
    umn_Token token;

    struct umn_PToken *lvalue, *rvalue;
} umn_PToken;

typedef UMN_SLAB_T(umn_PToken) umn_PToken_Allocator;

umn_PToken *umn_ptoken_alloc(umn_PToken_Allocator *allocator, const umn_Token *token)
{
    umn_PToken *ptoken = umn_slab_alloc(*allocator);
    assert(ptoken);

    if (token)
        ptoken->token = *token;

    return ptoken;
}

const char *umn_ptoken_strncpy(const umn_Lexer *lexer, const umn_PToken *ptoken, char *dest, size_t dsize)
{
    static const bool always_bracket = true;
    typedef struct
    {
        const umn_PToken *ptoken;
        int state;
    } stack_item_t;
    struct
    {
        size_t count, capacity;
        stack_item_t items[64];
    } stack = {.capacity = ARRAY_LEN(stack.items)};

    char buffer[256] = {0};

    size_t n = 0;
    stack_item_t item;

    umn_slice_push(stack, ((stack_item_t){ptoken}));

    while (stack.count)
    {
        item = umn_slice_pop(stack);
        ptoken = item.ptoken;

        if (ptoken->token.kind & UMN_KBINOP && item.state == 0) /* untouched */
        {
            umn_slice_push(stack, ((stack_item_t){ptoken, 1}));
            umn_slice_push(stack, ((stack_item_t){ptoken->lvalue}));

            if (always_bracket || ptoken->token.kind & UMN_KBRACK)
                n += snprintf(dest + n, dsize - n, "("); /* avoid directly touching the data */
        }
        else if (ptoken->token.kind & UMN_KBINOP && item.state == 1) /* untouched */
        {
            umn_slice_push(stack, ((stack_item_t){ptoken, 2}));
            umn_slice_push(stack, ((stack_item_t){ptoken->rvalue}));

            n += snprintf(dest + n, dsize - n, " %s ",
                          umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));
        }
        else if (ptoken->token.kind & UMN_KBINOP && item.state == 2)
        {
            if (always_bracket || ptoken->token.kind & UMN_KBRACK)
                n += snprintf(dest + n, dsize - n, ")");
        }
        else if (ptoken->token.kind & UMN_KFNCDEF) /* FNC APPLICATIONS will need a different logic */
        {
            n += snprintf(dest + n, dsize - n, "%s(", umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));

            /* NOTE: what happens when i allow for expressions, I guess not my current problem */

            for (umn_PToken *child = ptoken->lvalue; child; child = child->rvalue)
                n += snprintf(dest + n, dsize - n, child->rvalue ? "%s, " : "%s",
                              umn_token_strncpy(lexer, &child->token, dest + n, dsize - n));

            n += snprintf(dest + n, dsize - n, ")");
        }
        else
        {
            n += snprintf(dest + n, dsize - n, "%s",
                          umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));
        }
    }

    return dest;
}

void umn_ptoken_tree_print(const umn_Lexer *lexer, const umn_PToken *ptoken)
{
    typedef struct
    {
        const umn_PToken *ptoken;
        size_t depth_map;
        bool is_last;
    } stack_item_t;
    struct
    {
        size_t count, capacity;
        stack_item_t items[64];
    } stack = {.capacity = ARRAY_LEN(stack.items)};

    int depth;
    stack_item_t item;

    umn_slice_push(stack, ((stack_item_t){ptoken}));
    while (stack.count)
    {
        item = umn_slice_pop(stack);

        for (depth = 0; (item.depth_map >> depth); depth++)
            (void)NULL; /* do nothing */

        for (int i = 0; depth && i < (depth - 1); i++)
        {
            printf((item.depth_map & (1 << i)) ? "│  " : "   ");
        }
        if (depth > 0)
        {
            printf(item.is_last
                       ? "└──"
                       : "├──");

            /* no clue what this does */
            if (((umn_slice_at(stack, -1).depth_map & (1 << (depth - 1))) == 0))
                item.depth_map ^= 1UL << (depth - 1);
        }

        /* */
        if (item.ptoken == NULL)
        {
            printf("(nil)\n");
            continue;
        }
        umn_token_print(lexer, &item.ptoken->token);

        item.depth_map |= 1 << depth;
        if (item.ptoken->token.kind & UMN_KBINOP)
        {
            umn_slice_push(stack, ((stack_item_t){item.ptoken->rvalue, item.depth_map, true}));
            umn_slice_push(stack, ((stack_item_t){item.ptoken->lvalue, item.depth_map}));
        }
        else if ((item.ptoken->token.kind & UMN_KBINOP))
        {
            for (umn_PToken *child = item.ptoken->rvalue; child; child = child->lvalue)
                umn_slice_push(stack, ((stack_item_t){child, item.depth_map, child->rvalue == NULL}));
        }
    }
}

/* compute would be client side logic */

umn_PToken *umn_expr_parse(umn_PToken_Allocator *pallocator, umn_Lexer *lexer)
{
    umn_PToken *ptoken;
    umn_Token token;

    struct
    {
        size_t count, capacity;
        umn_PToken *items[64];
    } stack = {.capacity = ARRAY_LEN(stack.items)};

    struct
    {
        size_t count, capacity;
        unsigned items[ARRAY_LEN(stack.items) >> 2];
    } bstack = {.capacity = ARRAY_LEN(bstack.items)}; /* scope stack */

    bool expect_value = true;

    umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL));
    umn_slice_push(bstack, stack.count);

    while (umn_lexer_next(lexer, &token) == 0)
    {
        if (!expect_value && umn_token_issymbol(lexer, &token, ","))
        {
            umn_lexer_give(lexer, &token); /* return the token to the lexer */
            break;
        }

        if (expect_value && umn_token_issymbol(lexer, &token, "("))
        {
            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL)) == NULL)
                UMN_TODO("handle: memory error");
            umn_slice_push(bstack, stack.count);
            continue;
        }
        else if (!expect_value && umn_token_issymbol(lexer, &token, ")") && bstack.count > 0)
        {
            unsigned base_count = umn_slice_pop(bstack);
            ptoken = umn_slice_at(stack, base_count - 2);

            for (unsigned i = base_count - 1; i < (stack.count - 1); i++)
            {
                assert(umn_slice_at(stack, i)->rvalue == NULL);
                umn_slice_at(stack, i)->rvalue = umn_slice_at(stack, i + 1);
            }

            if (umn_slice_at(stack, base_count - 1)->token.kind == 0)
            {
                assert(umn_slice_at(stack, base_count - 1)->lvalue);

                void *tmp = umn_slice_at(stack, base_count - 1)->lvalue;
                memcpy(umn_slice_at(stack, base_count - 1), tmp, sizeof(umn_PToken));
                umn_slab_drop((*pallocator), tmp);

                if (ptoken->lvalue == NULL)
                    ptoken->lvalue = umn_slice_at(stack, base_count - 1);
                else if (ptoken->rvalue == NULL)
                    ptoken->rvalue = umn_slice_at(stack, base_count - 1);
            }
            else if (ptoken->token.kind == 0)
            {
                /* leaking a node but oh-well */
                memcpy(ptoken, umn_slice_at(stack, base_count - 1), sizeof(umn_PToken));
                ptoken->token.kind |= UMN_KBRACK;
            }
            else
            {
                assert(ptoken->rvalue == NULL);
                ptoken->rvalue = umn_slice_at(stack, base_count - 1);
                ptoken->rvalue->token.kind |= UMN_KBRACK;
            }

            stack.count = base_count - 1;
            continue;
        }
        else if (!expect_value && (token.kind == UMN_KSYMBOL && (token.flags & UMN_SF_BINOP)) && umn_slice_at(stack, -1)->token.kind == 0)
        {
            token.kind |= UMN_KBINOP;
            umn_slice_at(stack, -1)->token = token;
        }
        else if (!expect_value && (token.kind == UMN_KSYMBOL && (token.flags & UMN_SF_BINOP)))
        {
            unsigned base_count = umn_slice_at(bstack, -1);

            if (stack.count >= base_count && umn_slice_at(stack, -1)->token.data > token.data)
            {
                for (unsigned i = base_count - 1; i < (stack.count - 1); i++)
                {
                    assert(umn_slice_at(stack, i)->rvalue == NULL);
                    umn_slice_at(stack, i)->rvalue = umn_slice_at(stack, i + 1);
                }
                stack.count = base_count;
            }

            token.kind |= UMN_KBINOP;
            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, &token)) == NULL)
                UMN_TODO("handle: memory error");

            /* token.data is the operators precedence */
            if ((umn_slice_at(stack, -2)->token.kind & UMN_KBRACK))
            {
                assert(umn_slice_at(stack, -2)->rvalue);
                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2);
                umn_slice_at(stack, -2) = umn_slice_at(stack, -1);
                stack.count--;
            }
            else if (umn_slice_at(stack, -2)->token.data < umn_slice_at(stack, -1)->token.data)
            { /* higher prec */
                assert(umn_slice_at(stack, -2)->rvalue);

                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2)->rvalue;
                umn_slice_at(stack, -2)->rvalue = NULL;
            }
            else if (umn_slice_at(stack, -2)->token.data >= umn_slice_at(stack, -1)->token.data)
            { /* lower equal prec */
                /* wait until what happens when the brackets get involved */
                assert(umn_slice_at(stack, -2)->rvalue);

                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2);
                umn_slice_at(stack, -2) = umn_slice_at(stack, -1);
                stack.count--;
            }
        }
        else if (expect_value && (token.kind & UMN_KSYMBOL) == 0)
        {
            if ((ptoken = umn_ptoken_alloc(pallocator, &token)) == NULL)
                UMN_TODO("handle: memory error");

            if (umn_slice_at(stack, -1)->lvalue)
                umn_slice_at(stack, -1)->rvalue = ptoken;
            else
                umn_slice_at(stack, -1)->lvalue = ptoken;
        }
        else
        {
            /* dump tokens */
            puts("error");
            umn_token_print_error(lexer, &token);
            for (unsigned i = 0; i < stack.count; i++)
                umn_ptoken_tree_print(lexer, umn_slice_at(stack, i));

            token.kind |= UMN_KERR;
            return umn_ptoken_alloc(pallocator, &token);
        }

        expect_value = !expect_value;
    }

    if (stack.count == 0)
        return NULL;

    /* let's just go in an ascending order */
    for (unsigned i = 0; i < (stack.count - 1); i++)
    {
        assert(umn_slice_at(stack, i)->rvalue == NULL);
        umn_slice_at(stack, i)->rvalue = umn_slice_at(stack, i + 1);
    }

    if (umn_slice_at(stack, 0)->token.kind == 0 && umn_slice_at(stack, 0)->lvalue)
    {
        /* leaking a ptoken*/
        umn_slice_at(stack, 0) = umn_slice_at(stack, 0)->lvalue;
    }

    assert(umn_slice_at(stack, 0)->token.kind);

    return umn_slice_at(stack, 0);
}


#endif
