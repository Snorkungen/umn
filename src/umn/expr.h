#ifndef UMN_EXPR_H
#define UMN_EXPR_H

#include <stdarg.h>

#include "utils.h"
#include "lexer.h"

/*
    DEFAULT SYMBOLS TO GET GOING

    static const umn_Symbol umn_expr_symbols__[] = {
        {.s = "."},
        {.s = ",", .flags = UMN_SF_BARRR },
        {.s = "("},
        {.s = ")"},
        {.s = "=", .flags = UMN_SF_BINOP, .data = 16},
        {.s = "+", .flags = UMN_SF_BINOP, .data = 18},
        {.s = "*", .flags = UMN_SF_BINOP, .data = 19},
        {.s = "**", .flags = UMN_SF_BINOP, .data = 20},
    };
    static const umn_Lexer_Symbols umn_expr_symbols = {
        .count = ARRAY_LEN(umn_expr_symbols__),
        .items = umn_expr_symbols__,
    };

*/

static const umn_Token_Kind UMN_KBINOP = UMN_K__1,
                            UMN_KUNARY_L = UMN_K__2,
                            UMN_KUNARY_R = UMN_K__3,
                            UMN_KUNARY__ = UMN_KUNARY_L | UMN_KUNARY_R,
                            UMN_KBRACK = UMN_K__4,
                            UMN_KFNC = UMN_K__5,
                            UMN_KPARSE_ERR = UMN_K__8;

static const uint32_t UMN_SF_BINOP = 0x80;
static const uint32_t UMN_SF_UNARY_L = 0x80 << 1; /* The symbol is an unary operator (-1) operating on the left side. */
static const uint32_t UMN_SF_UNARY_R = 0x80 << 2; /* The symbol is an unary operator (1-) operating on the right side. */
static const uint32_t UMN_SF_BARRR = 0x80 << 8;   /* The sybmol is a barrier, where the tree builder, can fail succesfully */

/* parse token is an extension of a token with */
typedef struct umn_PToken
{
    umn_Token token;

    struct umn_PToken *lvalue, *rvalue;
    struct umn_PToken *prev, *next;
} umn_PToken;

typedef UMN_SLAB_T(umn_PToken) umn_PToken_Allocator;

static inline bool umn_lexer_peek_token_issymbol(const umn_Lexer *lexer, const char *literal);

umn_PToken *umn_ptoken_alloc(umn_PToken_Allocator *allocator, const umn_Token *token);
const char *umn_ptoken_strncpy(const umn_Lexer *lexer, const umn_PToken *ptoken, char *dest, size_t dsize);
void umn_ptoken_tree_print(const umn_Lexer *lexer, const umn_PToken *ptoken);

/* Append to a linked list of ptokens */
static umn_PToken *umn_ptoken_ll_append(umn_PToken *ll, umn_PToken *ptoken);

typedef struct
{
    size_t count, capacity;
    umn_PToken *items[32]; /* Fixed stack */
} umn_expr_parse_stack_t;

struct umn_expr_parsers_t;
typedef umn_PToken *umn_expr_parser_t(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *data, struct umn_expr_parsers_t value_parsers, const umn_Token *token);

typedef struct umn_expr_parsers_t
{
    size_t count, capacity;
    umn_expr_parser_t **items;
} umn_expr_parsers_t;

typedef umn_PToken *umn_expr_parse_value_t(umn_PToken_Allocator *pallocator, umn_Lexer *lexer, const umn_Token *token);
umn_PToken *umn_expr_parse_ext(umn_PToken_Allocator *pallocator, umn_Lexer *lexer, umn_expr_parse_value_t *parse_value_fptr);
umn_PToken *umn_expr_parse(umn_PToken_Allocator *pallocator, umn_Lexer *lexer);

umn_PToken *umn_expr_parse2(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parser_t *parser, ...);
umn_PToken *umn_expr_parse_rcrs(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers);

#ifdef UMN_EXPR_IMPLEMENTATION
#undef UMN_EXPR_IMPLEMENTATION

static inline bool umn_lexer_peek_token_issymbol(const umn_Lexer *lexer, const char *literal)
{
    umn_Token token;
    if (umn_lexer_peek(lexer, &token))
        return false;

    return umn_token_issymbol(lexer, &token, literal);
}

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
    static const bool always_bracket = false | true;
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
        else if (ptoken->token.kind & UMN_KUNARY_L)
        {
            umn_slice_push(stack, ((stack_item_t){ptoken->rvalue}));
            n += snprintf(dest + n, dsize - n, "%s",
                          umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));
        }
        else if (ptoken->token.kind & UMN_KUNARY_R && item.state == 0)
        {
            umn_slice_push(stack, ((stack_item_t){ptoken, 1}));
            umn_slice_push(stack, ((stack_item_t){ptoken->lvalue}));
        }
        else if (ptoken->token.kind & UMN_KUNARY_R && item.state == 1)
        {
            n += snprintf(dest + n, dsize - n, "%s",
                          umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));
        }
        else if (ptoken->token.kind & UMN_KFNC) /* FNC APPLICATIONS will need a different logic */
        {
            n += snprintf(dest + n, dsize - n, "%s(", umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));

            /* NOTE: what happens when i allow for expressions, I guess not my current problem */

            for (umn_PToken *child = ptoken->lvalue; child; child = child->rvalue)
                n += snprintf(dest + n, dsize - n, child->rvalue ? "%s, " : "%s",
                              umn_token_strncpy(lexer, &child->token, dest + n, dsize - n));

            n += snprintf(dest + n, dsize - n, ")");
        }
        else if (ptoken->token.kind == UMN_KLITERAL && ptoken->prev)
        {
            for (const umn_PToken *child = ptoken; child; child = child->next)
                n += snprintf(dest + n, dsize - n, "%s.",
                              umn_token_strncpy(lexer, &child->token, buffer, sizeof(buffer)));
            n--; /* remove trailing . */
        }
        else
        {
            n += snprintf(dest + n, dsize - n, "%s",
                          umn_token_strncpy(lexer, &ptoken->token, buffer, sizeof(buffer)));
        }
    }

    /* Add trailing null byte */
    dest[n < dsize ? n : dsize - 1] = '\0';

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
        if (item.ptoken->token.kind & UMN_KBINOP || (item.ptoken->token.kind & UMN_KPARSE_ERR) || item.ptoken->token.kind == 0)
        {
            umn_slice_push(stack, ((stack_item_t){item.ptoken->rvalue, item.depth_map, true}));
            umn_slice_push(stack, ((stack_item_t){item.ptoken->lvalue, item.depth_map}));
        }
        else if (item.ptoken->token.kind & UMN_KUNARY_L)
        {
            umn_slice_push(stack, ((stack_item_t){item.ptoken->rvalue, item.depth_map, true}));
        }
        else if (item.ptoken->token.kind & UMN_KUNARY_R)
        {
            umn_slice_push(stack, ((stack_item_t){item.ptoken->lvalue, item.depth_map, true}));
        }
        else if ((item.ptoken->token.kind & UMN_KFNC))
        {
            for (umn_PToken *child = item.ptoken->next; child; child = child->prev)
                umn_slice_push(stack, ((stack_item_t){child, item.depth_map, child->next == NULL || item.ptoken->next == child}));
        }
    }
}

static inline umn_PToken *umn_ptoken_ll_append(umn_PToken *ll, umn_PToken *ptoken)
{
    if (ll->next == NULL)
    {
        ll->prev = ll->next = ptoken;
    }
    else
    {
        ptoken->prev = ll->next;
        ll->next->next = ptoken;
        ll->next = ptoken;
    }
    return ll->next;
}

static umn_PToken *umn_expr_parse__set_value(umn_PToken *ptoken, umn_PToken *value);
static umn_PToken *umn_expr_parse__eval_stack(umn_PToken_Allocator *pallocator, size_t count, umn_PToken *items[]);
static umn_PToken *umn_expr_parse__dot_sep_literal(umn_PToken_Allocator *pallocator, umn_Lexer *lexer, umn_Token *token);

static void umn_expr_parse__fold_stack(umn_expr_parse_stack_t *stack);

umn_expr_parser_t umn_expr_parse__func_value; /* SHOULD THIS FUNCTIONALITY BE GENERAL ?? */
umn_expr_parser_t umn_expr_parse__bracket_value;
umn_expr_parser_t umn_expr_parse__value;

umn_PToken *umn_expr_parse2(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parser_t *parser, ...)
{
    umn_expr_parser_t *items[16] = {0};
    umn_expr_parsers_t parsers = {
        .items = items,
        .capacity = ARRAY_LEN(items),
    };

    if (parser)
    {
        va_list args;
        va_start(args, parser);
        umn_slice_push(parsers, parser);

        while ((parser = va_arg(args, umn_expr_parser_t *)))
            umn_slice_push(parsers, parser);

        va_end(args);
    }

    return umn_expr_parse_rcrs(lexer, ptoken_allocator, value_parser_data, parsers);
}

umn_PToken *umn_expr_parse_rcrs(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers)
{
    bool expect_value = true;
    umn_PToken *ptoken, *err_ptoken;
    umn_Token token;

    umn_expr_parse_stack_t stack = {.capacity = ARRAY_LEN(stack.items)};

    umn_slice_push(stack, umn_ptoken_alloc(ptoken_allocator, NULL));
    assert(stack.items[0]);

    while (umn_lexer_next(lexer, &token) == 0 && token.kind)
    { /* Have the loop that does stuff here ... */

        if (!expect_value && token.kind == UMN_KSYMBOL && token.flags & UMN_SF_BINOP)
        {
            token.kind |= UMN_KBINOP;

            if (umn_slice_at(stack, -1)->token.kind == 0)
            {
                umn_slice_at(stack, -1)->token = token;
            }
            else
            {
                ptoken = umn_ptoken_alloc(ptoken_allocator, &token);
                assert(ptoken);
                assert(umn_slice_at(stack, -1)->rvalue);

                if (umn_slice_at(stack, -1)->token.kind & (UMN_KUNARY__ | UMN_KBRACK) || umn_slice_at(stack, -1)->token.data > token.data)
                { /* stack has a higher precedence item in it */
                    umn_expr_parse__fold_stack(&stack);

                    ptoken->lvalue = umn_slice_at(stack, -1);
                    umn_slice_at(stack, -1) = ptoken;
                }
                else
                { /* stack has a lower precedence in it*/
                    ptoken->lvalue = umn_slice_at(stack, -1)->rvalue;
                    umn_slice_at(stack, -1)->rvalue = NULL;

                    /* could try to recover with and intermediate evaluation of the stack and stuff */
                    assert(stack.count != stack.capacity);

                    umn_slice_push(stack, ptoken);
                }
            }
        }
        else if (!expect_value && token.kind == UMN_KSYMBOL && token.flags & UMN_SF_UNARY_R)
        {
            token.kind |= UMN_KUNARY_R; /* */

            if (umn_slice_at(stack, -1)->token.kind == 0)
            {
                assert(umn_slice_at(stack, -1)->lvalue);
                assert(umn_slice_at(stack, -1)->rvalue == NULL);

                umn_slice_at(stack, -1)->token = token;
            }
            else
            {
                assert(umn_slice_at(stack, -1)->token.kind);
                assert(umn_slice_at(stack, -1)->rvalue);

                ptoken = umn_ptoken_alloc(ptoken_allocator, &token);
                assert(ptoken);

                ptoken->lvalue = umn_slice_at(stack, -1)->rvalue;
                umn_slice_at(stack, -1)->rvalue = ptoken;
            }

            continue;
        }
        else if (expect_value && token.kind == UMN_KSYMBOL && token.flags & UMN_SF_UNARY_L)
        {
            token.kind |= UMN_KUNARY_L; /* */
            if (umn_slice_at(stack, -1)->token.kind == 0)
                umn_slice_at(stack, -1)->token = token;
            else
                assert(umn_slice_push(stack, umn_ptoken_alloc(ptoken_allocator, &token)));

            continue;
        }
        else if (expect_value && ((ptoken = umn_expr_parse__bracket_value(lexer, ptoken_allocator, value_parser_data, value_parsers, &token)) ||
                                  (ptoken = umn_expr_parse__value(lexer, ptoken_allocator, value_parser_data, value_parsers, &token))))
        { /* parse bracketted expressions and custom values */
            if (ptoken->token.kind & (UMN_KERR | UMN_KPARSE_ERR))
                return ptoken;

            umn_expr_parse__set_value(umn_slice_at(stack, -1), ptoken);
        }
        else if (expect_value && ((token.kind & UMN_KNUMERIC) || token.kind == UMN_KLITERAL || token.kind == UMN_KSTRING))
        { /* parse default value_types */
            ptoken = umn_ptoken_alloc(ptoken_allocator, &token);
            assert(ptoken);
            umn_expr_parse__set_value(umn_slice_at(stack, -1), ptoken);
        }
        else
        {
            /* handle errors and stuff ... */
            if (!expect_value && token.flags & UMN_SF_BARRR)
            {
                umn_lexer_give(lexer, &token);
                break;
            }

            {
                err_ptoken = umn_ptoken_alloc(ptoken_allocator, &token);
                err_ptoken->token.kind = UMN_KPARSE_ERR | UMN_KERR;
                err_ptoken->lvalue = umn_slice_at(stack, -1);
                err_ptoken->rvalue = umn_ptoken_alloc(ptoken_allocator, &token);
                return err_ptoken;
            }
        }

        expect_value = !expect_value;
    }

    if (expect_value || umn_slice_at(stack, -1)->rvalue == NULL)
    {
        err_ptoken = umn_ptoken_alloc(ptoken_allocator, &token);
        err_ptoken->token.kind = UMN_KPARSE_ERR | UMN_KERR;
        err_ptoken->lvalue = umn_slice_at(stack, -1);
    }

    umn_expr_parse__fold_stack(&stack);
    ptoken = stack.items[0];

    if (ptoken->token.kind == 0)
    {
        if (ptoken->lvalue)
        {
            void *tmp = ptoken->lvalue;
            (*ptoken) = (*ptoken->lvalue);
            umn_slab_drop((*ptoken_allocator), tmp); /* does this optimization actually achieve something useful */
            return ptoken;
        }
        else
        { 
            /* or should this thing return an error token with no more data left ...*/
            umn_slab_drop((*ptoken_allocator), ptoken);
            return NULL;
        }
    }

    return ptoken;
}

umn_PToken *umn_expr_parse__bracket_value(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers, const umn_Token *token)
{
    if (!umn_token_issymbol(lexer, token, "("))
        return NULL;

    umn_PToken *ptoken = umn_expr_parse_rcrs(lexer, ptoken_allocator, value_parser_data, value_parsers);

    if (ptoken->token.kind & (UMN_KBINOP))
        ptoken->token.kind |= UMN_KBRACK;

    /* NOTE: this function is special thus it is allowed to modify the token */
    if (umn_lexer_next(lexer, (umn_Token *)token) || !umn_token_issymbol(lexer, token, ")"))
        return NULL;

    return ptoken;
}

umn_PToken *umn_expr_parse__value(umn_Lexer *src_lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, const umn_expr_parsers_t value_parsers, const umn_Token *src_token)
{
    umn_Lexer lexer;
    umn_Token token;
    umn_PToken *ptoken;

    for (int i = 0; i < value_parsers.count; i++)
    {
        lexer = *src_lexer;
        token = *src_token;
        ptoken = value_parsers.items[i](&lexer, ptoken_allocator, value_parser_data, value_parsers, &token);

        if (ptoken)
        {
            *src_lexer = lexer;
            return ptoken;
        }

        assert(src_token->begin == token.begin); /* do not allow for the token to be modified */
    }

    return NULL;
}

umn_PToken *umn_expr_parse__func_value(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers, const umn_Token *src_token)
{
    umn_Token token;

    if (src_token->kind != UMN_KLITERAL || !(umn_lexer_peek(lexer, &token) == 0 && umn_token_issymbol(lexer, &token, "(")))
        return NULL;

    umn_lexer_take(lexer, &token);

    /* init the ptoken */
    umn_PToken *ptoken = umn_ptoken_alloc(ptoken_allocator, src_token), *value_ptoken;
    assert(ptoken);
    ptoken->token.kind |= UMN_KFNC;

    bool skipped = false;

    while (umn_lexer_next(lexer, &token) == 0 && token.kind)
    {
        if (umn_token_issymbol(lexer, &token, ")"))
        {
            return ptoken;
        }
        else if (umn_token_issymbol(lexer, &token, ","))
        {
            if (skipped)
                break;

            skipped = true;
            continue;
        }

        umn_lexer_give(lexer, &token);
        value_ptoken = umn_expr_parse_rcrs(lexer, ptoken_allocator, value_parser_data, value_parsers);
        assert(value_ptoken); /* when would this return null ?? */

        if (value_ptoken->token.kind & (UMN_KERR | UMN_KPARSE_ERR))
        {
            return value_ptoken; /* CONSIDER A BETTER WAY OF DOING THIS :::: */
        }

        umn_ptoken_ll_append(ptoken, value_ptoken);
        skipped = false;
    }

    /* error_ here ... */

    umn_PToken *err_ptoken = umn_ptoken_alloc(ptoken_allocator, NULL);
    err_ptoken->token.kind = UMN_KPARSE_ERR | UMN_KERR;
    err_ptoken->lvalue = ptoken;
    err_ptoken->rvalue = umn_ptoken_alloc(ptoken_allocator, &token);

    return err_ptoken;
}

umn_PToken *umn_expr_parse_ext(umn_PToken_Allocator *pallocator, umn_Lexer *lexer, umn_expr_parse_value_t *parse_value_fptr)
{
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

    struct
    {
        size_t count, capacity;
        unsigned items[16]; /* this is a bstack count */
    } exprs_stack = {.capacity = ARRAY_LEN(exprs_stack.items)};

    umn_PToken *ptoken;
    umn_Token token;

    bool expect_value = true;

    umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL));
    umn_slice_push(bstack, stack.count);

    while (umn_lexer_next(lexer, &token) == 0 && token.kind)
    {

        if (!expect_value && exprs_stack.count > 0 && umn_slice_at(exprs_stack, -1) == bstack.count && umn_token_issymbol(lexer, &token, ","))
        {
            unsigned stack_offset = umn_slice_at(bstack, umn_slice_at(exprs_stack, -1) - 1) - 1;
            ptoken = umn_expr_parse__eval_stack(pallocator, stack.count - stack_offset, stack.items + stack_offset);
            assert(ptoken);

            umn_ptoken_ll_append(umn_slice_at(stack, stack_offset - 1), ptoken);

            stack.count = stack_offset;
            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL)) == NULL)
                UMN_TODO("handle: memory error");
        }
        else if (exprs_stack.count > 0 && umn_slice_at(exprs_stack, -1) == bstack.count && umn_token_issymbol(lexer, &token, ")"))
        { /*  commit the most recent thing and end the expected series */
            unsigned stack_offset = umn_slice_at(bstack, umn_slice_at(exprs_stack, -1) - 1) - 1;

            if (expect_value)
            {
                assert(umn_slice_at(stack, stack_offset)->token.kind == 0 && umn_slice_at(stack, stack_offset)->lvalue == NULL);
                umn_slab_drop((*pallocator), umn_slice_at(stack, stack_offset));
            }
            else
            {
                ptoken = umn_expr_parse__eval_stack(pallocator, stack.count - stack_offset, stack.items + stack_offset);
                assert(ptoken);

                umn_ptoken_ll_append(umn_slice_at(stack, stack_offset - 1), ptoken);
            }

            stack.count = stack_offset; /* reset the stack count */

            umn_slice_pop(bstack);
            umn_slice_pop(exprs_stack);
            umn_slice_pop(stack);

            expect_value = false;

            continue;
        }
        else if (expect_value && umn_token_issymbol(lexer, &token, "("))
        {
            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL)) == NULL)
                UMN_TODO("handle: memory error");
            umn_slice_push(bstack, stack.count);
            continue;
        }
        else if (!expect_value && bstack.count > 1 && umn_token_issymbol(lexer, &token, ")"))
        {
            unsigned stack_offset = umn_slice_pop(bstack) - 1;

            ptoken = umn_expr_parse__eval_stack(pallocator, stack.count - stack_offset, stack.items + stack_offset);
            stack.count = stack_offset;

            assert(ptoken); /* since this branch asserts that the state has already received a value otherwise `expect_value` would be truthy */

            if (ptoken->token.kind & UMN_KBINOP)
                ptoken->token.kind |= UMN_KBRACK;

            if (ptoken->token.kind & UMN_KBINOP && umn_slice_at(stack, stack_offset - 1)->token.kind & UMN_KBINOP && umn_slice_at(stack, stack_offset - 1)->lvalue == NULL)
            {
                /* NOTE: leaking a ptoken  */
                memcpy(umn_slice_at(stack, stack_offset - 1), ptoken, sizeof(*ptoken));
                /*
                    TODO: solve the ptoken leak
                    allocated       (nil),  (=),    (x),    (1)
                    after memcpy    (=),    (=),    (x),    (1)
                                     0       1       2       3

                    clone the tree
                    starting from index 0

                    a way of doing this is create a copy of  pallocator and drop starting from 0
                    then knowing that we own the pallocator and then copy willy nilly
                */
            }
            else
            {
                umn_expr_parse__set_value(umn_slice_at(stack, stack_offset - 1), ptoken);
            }

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
            if ((umn_slice_at(stack, -2)->token.kind & (UMN_KBRACK | UMN_KUNARY_L)) || umn_slice_at(stack, -2)->token.data >= umn_slice_at(stack, -1)->token.data)
            { /* The new operator has a lower precedence */
                assert(umn_slice_at(stack, -2)->rvalue);
                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2);
                umn_slice_at(stack, -2) = umn_slice_at(stack, -1);
                stack.count--;
            }
            else if (umn_slice_at(stack, -2)->token.data < umn_slice_at(stack, -1)->token.data)
            { /* The new operator has a higher precedence */
                assert(umn_slice_at(stack, -2)->rvalue);

                umn_slice_at(stack, -1)->lvalue = umn_slice_at(stack, -2)->rvalue;
                umn_slice_at(stack, -2)->rvalue = NULL;
            }
        }
        else if (expect_value && (token.kind == UMN_KSYMBOL && (token.flags & UMN_SF_UNARY_L)))
        {
            token.kind |= UMN_KUNARY_L;

            if (umn_slice_at(stack, -1)->token.kind == 0)
                umn_slice_at(stack, -1)->token = token;
            else
            {
                if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, &token)) == NULL)
                    UMN_TODO("handle: memory error");
            }

            continue;
        }
        else if (expect_value && parse_value_fptr && (ptoken = parse_value_fptr(pallocator, lexer, &token)))
        {
            /* TODO: come back and look at what I'm attempting to do */
            if (token.kind & UMN_KERR)
                break;
            else if ((ptoken->token.kind & UMN_KPARSE_ERR))
                return ptoken; /* the thing can figure out the parse error */

            umn_expr_parse__set_value(umn_slice_at(stack, -1), ptoken);
        }
        else if (expect_value && (token.kind == UMN_KLITERAL) && umn_lexer_peek_token_issymbol(lexer, "("))
        { /* parse the function thing ... */
            assert(exprs_stack.count < exprs_stack.capacity && ("exprs_stack overflow"));

            /* push the the function thing */
            token.kind |= UMN_KFNC;
            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, &token)) == NULL)
                UMN_TODO("handle: memory error");

            /* already set the function as the thing ... */
            umn_expr_parse__set_value(umn_slice_at(stack, -2), umn_slice_at(stack, -1));

            if (umn_slice_push(stack, umn_ptoken_alloc(pallocator, NULL)) == NULL)
                UMN_TODO("handle: memory error");

            umn_slice_push(bstack, stack.count);
            umn_slice_push(exprs_stack, bstack.count);

            umn_lexer_next(lexer, &token);
            continue; /* avoid the next iter expects a value */
        }
        else if (expect_value && (token.kind == UMN_KLITERAL && umn_lexer_peek_token_issymbol(lexer, ".") && (ptoken = umn_expr_parse__dot_sep_literal(pallocator, lexer, &token)))) /* or should this be a function*/
        {
            umn_expr_parse__set_value(umn_slice_at(stack, -1), ptoken);
        }
        else if (expect_value && ((token.kind & UMN_KNUMERIC) || token.kind == UMN_KLITERAL || token.kind == UMN_KSTRING))
        {
            if ((ptoken = umn_ptoken_alloc(pallocator, &token)) == NULL)
                UMN_TODO("handle: memory error");

            umn_expr_parse__set_value(umn_slice_at(stack, -1), ptoken);
        }
        else
        {
            /* see if this can still resolve a value and stuff */
            if (stack.count > 1 && (umn_slice_at(stack, -1)->token.kind & UMN_KBINOP) && umn_slice_at(stack, -1)->lvalue == NULL)
                umn_slab_drop((*pallocator), umn_slice_pop(stack)); /* drop the empty token and stuff */

            if (bstack.count == 1 && (umn_slice_at(stack, -1)->rvalue || (umn_slice_at(stack, -1)->lvalue && umn_slice_at(stack, -1)->token.kind == 0)))
            { /* TODO: figure out a better way to exit successfully when the expression has been parsed */
                if ((token.kind & (UMN_KSYMBOL & ~UMN_KLITERAL)) == 0 || (token.kind == UMN_KSYMBOL && (bool)(token.flags & UMN_SF_BARRR)))
                {
                    umn_lexer_give(lexer, &token);
                    break;
                }
            }

            {
                umn_PToken *err_ptoken = umn_ptoken_alloc(pallocator, &token);
                err_ptoken->token.kind = UMN_KPARSE_ERR | UMN_KERR;
                err_ptoken->lvalue = umn_slice_at(stack, -1);
                err_ptoken->rvalue = umn_ptoken_alloc(pallocator, &token);
                return err_ptoken;
            }
        }

        expect_value = !expect_value;
    }

    if (token.kind & UMN_KERR)
        return umn_ptoken_alloc(pallocator, &token);

    if (expect_value && stack.count > 1)
    {
        umn_PToken *err_ptoken = umn_ptoken_alloc(pallocator, &token);
        err_ptoken->token.kind = UMN_KPARSE_ERR | UMN_KERR;
        err_ptoken->lvalue = umn_slice_at(stack, -1);
        return err_ptoken;
    }

    return umn_expr_parse__eval_stack(pallocator, stack.count, stack.items);
}

inline umn_PToken *umn_expr_parse(umn_PToken_Allocator *pallocator, umn_Lexer *lexer)
{
    return umn_expr_parse_ext(pallocator, lexer, NULL);
}

/* @return value */
inline static umn_PToken *umn_expr_parse__set_value(umn_PToken *ptoken, umn_PToken *value)
{
    if (ptoken->lvalue == NULL && ((ptoken->token.kind & UMN_KUNARY_L) == 0))
        ptoken->lvalue = value;
    else
        ptoken->rvalue = value;

    return value;
}

inline static void umn_expr_parse__fold_stack(umn_expr_parse_stack_t *stack)
{
    for (int i = stack->count - 2; i >= 0; i--)
    {
        assert(umn_slice_at((*stack), i)->rvalue == NULL);
        assert(umn_slice_at((*stack), i + 1)->rvalue);

        umn_slice_at((*stack), i)->rvalue = umn_slice_at((*stack), i + 1);
    }

    stack->count = 1;
}

inline static umn_PToken *umn_expr_parse__eval_stack(umn_PToken_Allocator *pallocator, size_t count, umn_PToken *items[])
{
    for (int i = 0; i < (count - 1); i++)
    {
        assert(items[i]->rvalue == NULL);
        items[i]->rvalue = items[i + 1];
    }

    if (items[0]->token.kind == 0 && items[0]->lvalue)
    {
        void *to_be_dropped = items[0]->lvalue;
        memcpy(items[0], items[0]->lvalue, sizeof(*items[0]));
        umn_slab_drop((*pallocator), to_be_dropped);
    }

    if (items[0]->token.kind == 0)
        return NULL;

    return items[0];
}

/* parses a singly linked list of do seperated literals */
static umn_PToken *umn_expr_parse__dot_sep_literal(umn_PToken_Allocator *pallocator, umn_Lexer *lexer, umn_Token *token)
{
    umn_PToken *ptoken = NULL, *next = NULL;

    if ((ptoken = umn_ptoken_alloc(pallocator, token)) == NULL)
        UMN_TODO("allocator failed");

    while (umn_lexer_next(lexer, token) == 0 && umn_token_issymbol(lexer, token, "."))
    {
        if (umn_lexer_next(lexer, token) || token->kind != UMN_KLITERAL)
        {
            token->kind |= UMN_KERR;
            return NULL;
        }

        if ((next = umn_ptoken_alloc(pallocator, token)) == NULL)
            UMN_TODO("allocator failed");

        umn_ptoken_ll_append(ptoken, next);
    }

    assert(ptoken->prev); /* atleast one thing must get inserted into the linked list */
    return ptoken;
}

#endif
#endif
