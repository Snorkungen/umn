#ifndef UMN_EXPR_H
#define UMN_EXPR_H

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#include "./lexer.h"

static umn_Symbol umn_expr_symbols[] = {
    {","},
    {"("},
    {")"},
    {"+", .attrs.data = 1},
    {"*", .attrs.data = 2},
};

typedef struct
{
    int64_t lvalue, rvalue;
    size_t unit;
} umn_Expr_Node;

typedef struct
{
    size_t count, capacity;
    umn_Expr_Node *items;
} umn_Expr_Stack;

umn_Expr_Node *umn_expr_stack_allocate(umn_Expr_Stack *stack)
{
    assert(stack->count < stack->capacity);
    memset((stack->items + stack->count), 0, sizeof(*stack->items));
    return &stack->items[stack->count++];
}

/* 1st just figure out how to calculate, worry about allocating a binary tree */
int64_t umn_expr_compute__calculate(const umn_Expr_Node *node)
{
#define __calculate_compute__ 1
    switch (node->unit)
    {
    case 1:
#if __calculate_compute__
        printf("%ld + %ld\n", node->lvalue, node->rvalue);
#endif
        return node->lvalue + node->rvalue;
    case 2:
#ifdef __calculate_compute__
        printf("%ld * %ld\n", node->lvalue, node->rvalue);
#endif
        return node->lvalue * node->rvalue;
    }
#undef __calculate_compute__
    assert(0);
}

int64_t umn_expr_compute(char *inp)
{

    umn_Lexer lexer = {
        .data = inp,
        .data_len = strlen(inp),
        .symbols = umn_expr_symbols,
        .symbol_count = ARRAY_LEN(umn_expr_symbols),
    };

    umn_Token token;

    bool expect_value = true;
    umn_Expr_Node __nodes__[16] = {0};
    umn_Expr_Stack stack = {.items = __nodes__, .capacity = ARRAY_LEN(__nodes__)};
    umn_Expr_Node *curr = NULL;

    curr = umn_expr_stack_allocate(&stack); /* allocate a default thing */
    size_t base_count = stack.count;

    size_t __counts__[16] = {0};
    struct
    {
        size_t count, capacity;
        size_t *items;
    } counts = {.items = __counts__, .capacity = ARRAY_LEN(__counts__)};

    base_count = counts.items[counts.count++] = stack.count; /* push */

    while (umn_lexer_peek(&lexer, &token) == 0)
    {
        if (token.kind == UMN_KEOF)
            break;
        if (umn_token_issymbol(&lexer, &token, "("))
        {
            /* now I remember we must add empty things to pad the thing */
            curr = umn_expr_stack_allocate(&stack); /* just append an empty thing for the vibes */

            assert(counts.count < counts.capacity);
            base_count = counts.items[counts.count++] = stack.count /* push */;

            umn_lexer_take(&lexer, &token);
            continue;
        }
        else if (umn_token_issymbol(&lexer, &token, ")"))
        {
            assert(counts.count > 0);
            base_count = counts.items[--counts.count - 1]; /* pop */

            for (int i = base_count; i < stack.count && stack.items[i].unit > 0; i++)
            {
                (stack.items + i)->lvalue = stack.items[base_count].lvalue;
                stack.items[base_count].lvalue = umn_expr_compute__calculate((stack.items + i));
            }

            if (stack.items[base_count - 1].unit)
            {
                stack.items[base_count - 1].rvalue = stack.items[base_count].lvalue;
            }
            else
            {
                stack.items[base_count - 1].lvalue = stack.items[base_count].lvalue;
                stack.items[base_count - 1].unit = 0;
            }
            
            stack.count = base_count; /* reset, retain first element */
            curr = &stack.items[stack.count - 1];

            umn_lexer_take(&lexer, &token);
            continue;
        }
        else if (!expect_value)
        { /* expect a unit*/
            if (token.kind != UMN_KSYMBOL || token.d.symbol.data == 0)
            {
                token.kind |= UMN_KERR;
                break;
            }

            if (curr->unit > 0)
                curr = umn_expr_stack_allocate(&stack);

            curr->unit = token.d.symbol.data;
        }
        else if (expect_value)
        { /* expect value */
            if (token.kind != UMN_KINTEGER)
            {
                token.kind |= UMN_KERR;
                break;
            }

            if (stack.count == base_count && curr->unit == 0)
            {
                curr->lvalue = umn_token_readi(&lexer, &token);
                printf("lvalue = %ld\n", curr->lvalue);
            }
            else if (stack.count > (base_count) && (curr - 1)->unit < curr->unit)
            { /* the previous has a lower precedence, so current uses that ones rvalue */
                curr->lvalue = (curr - 1)->rvalue;
                curr->rvalue = umn_token_readi(&lexer, &token);
                (curr - 1)->rvalue = umn_expr_compute__calculate(curr);
                
                curr = &stack.items[(--stack.count) - 1]; /* pop the stack */
            }
            else if (stack.count > base_count)
            { /* the prec is lower so reduce the stack thing ... */
                for (int i = (base_count - 1); i < (stack.count - 1); i++)
                {
                    (stack.items + i)->lvalue = stack.items[base_count - 1].lvalue;
                    stack.items[base_count - 1].lvalue = umn_expr_compute__calculate((stack.items + i));
                }
                
                stack.count = base_count - 1; /* reset, retain first element */
                stack.items[stack.count].unit = curr->unit;
                stack.items[stack.count].rvalue = umn_token_readi(&lexer, &token);
                curr = &stack.items[stack.count++];
            }
            else
            {
                curr->rvalue = umn_token_readi(&lexer, &token);
                printf("rvalue = %ld\n", curr->rvalue);
            }
        }

        expect_value = !expect_value; /* flip flop */
        umn_lexer_take(&lexer, &token);
    }

    assert((token.kind & UMN_KERR) == 0);
    assert(stack.count > 0);

    for (int i = 0; i < stack.count && stack.items[i].unit > 0; i++)
    {
        (stack.items + i)->lvalue = stack.items[0].lvalue;
        stack.items[0].lvalue = umn_expr_compute__calculate((stack.items + i));
    }
    /* YOLO: this is not going to work first try ... PLS segfault */

    printf("%s = %lu\n", inp, stack.items[0].lvalue);

    return stack.items[0].lvalue;
}

#endif