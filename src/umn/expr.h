#ifndef UMN_EXPR_H
#define UMN_EXPR_H

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

umn_expr_parser_t umn_expr_parse__bracket_value;
umn_expr_parser_t umn_expr_parse__values;
umn_expr_parser_t umn_expr_parse__func_value; /* SHOULD THIS FUNCTIONALITY BE GENERAL ?? */

umn_PToken *umn_expr_parse(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parser_t *parser, ...);
umn_PToken *umn_expr_parse_ext(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers);
umn_PToken *umn_expr_parse_err_ptoken(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, const umn_Token *token, umn_PToken *context);

#ifdef UMN_EXPR_IMPLEMENTATION
#undef UMN_EXPR_IMPLEMENTATION

#include <stdarg.h>

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
static void umn_expr_parse__fold_stack(umn_expr_parse_stack_t *stack);

umn_PToken *umn_expr_parse(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parser_t *parser, ...)
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

    return umn_expr_parse_ext(lexer, ptoken_allocator, value_parser_data, parsers);
}

umn_PToken *umn_expr_parse_ext(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers)
{
    bool expect_value = true;
    umn_PToken *ptoken;
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
                                  (ptoken = umn_expr_parse__values(lexer, ptoken_allocator, value_parser_data, value_parsers, &token))))
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
            if (umn_slice_at(stack, -1)->rvalue || (umn_slice_at(stack, -1)->lvalue && umn_slice_at(stack, -1)->token.kind == 0))
            { /* TODO: figure out a better way to exit successfully when the expression has been parsed */
                if ((token.kind & (UMN_KSYMBOL & ~UMN_KLITERAL)) == 0 || (token.kind == UMN_KSYMBOL && (bool)(token.flags & UMN_SF_BARRR)))
                {
                    umn_lexer_give(lexer, &token);
                    break;
                }
            }

            return umn_expr_parse_err_ptoken(lexer, ptoken_allocator, &token, umn_slice_at(stack, -1));
        }

        expect_value = !expect_value;
    }

    if (stack.count == 1 && umn_slice_at(stack, 0)->lvalue == NULL && umn_slice_at(stack, 0)->token.kind == 0)
    {
        return NULL; /* if end of data then just return null */
    }
    else if (expect_value || (umn_slice_at(stack, 0)->token.kind & (UMN_KBINOP | UMN_KUNARY_L) && umn_slice_at(stack, -1)->rvalue == NULL))
    {
        return umn_expr_parse_err_ptoken(lexer, ptoken_allocator, NULL, umn_slice_at(stack, -1));
    }

    umn_expr_parse__fold_stack(&stack);
    ptoken = stack.items[0];

    if (ptoken->token.kind == 0)
    {
        if (ptoken->lvalue == NULL)
        {
            umn_slab_drop((*ptoken_allocator), ptoken);
            return NULL;
        }

        /* LEAKING A PNODE ... */
        return ptoken->lvalue;
    }

    return ptoken;
}

umn_PToken *umn_expr_parse__bracket_value(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, umn_expr_parsers_t value_parsers, const umn_Token *token)
{
    if (!umn_token_issymbol(lexer, token, "("))
        return NULL;

    umn_PToken *ptoken = umn_expr_parse_ext(lexer, ptoken_allocator, value_parser_data, value_parsers);

    if (ptoken->token.kind & (UMN_KBINOP))
        ptoken->token.kind |= UMN_KBRACK;

    /* NOTE: this function is special thus it is allowed to modify the token */
    if (umn_lexer_next(lexer, (umn_Token *)token) || !umn_token_issymbol(lexer, token, ")"))
        return NULL;

    return ptoken;
}

umn_PToken *umn_expr_parse__values(umn_Lexer *src_lexer, umn_PToken_Allocator *ptoken_allocator, void *value_parser_data, const umn_expr_parsers_t value_parsers, const umn_Token *src_token)
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
        value_ptoken = umn_expr_parse_ext(lexer, ptoken_allocator, value_parser_data, value_parsers);
        assert(value_ptoken); /* when would this return null ?? */

        if (value_ptoken->token.kind & (UMN_KERR | UMN_KPARSE_ERR))
        {
            /* NOTE: I need a way to bubble up the error .. */
            return value_ptoken; /* CONSIDER A BETTER WAY OF DOING THIS :::: */
        }

        umn_ptoken_ll_append(ptoken, value_ptoken);
        skipped = false;
    }

    return umn_expr_parse_err_ptoken(lexer, ptoken_allocator, NULL, ptoken);
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

umn_PToken *umn_expr_parse_err_ptoken(umn_Lexer *lexer, umn_PToken_Allocator *ptoken_allocator, const umn_Token *token, umn_PToken *context)
{
    umn_PToken *err_ptoken = umn_ptoken_alloc(ptoken_allocator, token);
    err_ptoken->token.kind |= UMN_KPARSE_ERR | UMN_KERR;
    err_ptoken->lvalue = context;
    err_ptoken->rvalue = umn_ptoken_alloc(ptoken_allocator, token);
    return err_ptoken;
}

#endif
#endif
