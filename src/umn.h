/*
    Read the U* Math Notation

    - [X] Read ASCII literals and keywords
    - [X] Read ASCII Integers
    - [X] Read 0x* 0b*
    - [X] decimal dot notation fraction *.*
        - [X] .*
    - [X] Pass on the keyword encoding and kinds somehow
    - [ ] Research utf-8
    - [ ] Allow underscored in integers ?? 100_00 fine 01_21 NONO 01'1 ?? 0x64'aa ??
    - [ ] Better error message passing, maybe point to a constant string in value ??

    - [ ] Big question should unary - be (-a) or (-1 * a)  becuase during this step just making sense of the tokens, which is different ...
*/

#ifndef UMN_H
#define UMN_H

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

/* BEGINING OF KIND MAGIC */

/* UMN_Kind is a bit field */
typedef size_t UMN_Kind;

/* define bit field structure */
#define UMN_KIND_BF_DATA (0xFFUL << (7 * 8)) /* this is a data mask */

#define UMN_KIND_BF_ERROR (0x80UL << (6 * 8))
#define UMN_KIND_BF_EOF (0x40UL << (6 * 8))
#define UMN_KIND_BF_NUMERIC (0x20UL << (6 * 8))
#define UMN_KIND_BF_LITERAL (0x10UL << (6 * 8))

#define UMN_KIND_BF_INTEGER (0x08UL << (6 * 8))
#define UMN_KIND_BF_FRACTION (0x04UL << (6 * 8))
#define UMN_KIND_BF_KEYWORD (0x02UL << (6 * 8))
#define UMN_KIND_BF_OPERATOR (0x01UL << (6 * 8))

#define UMN_KIND_BF_VARIABLE (0x80UL << (5 * 8))
#define UMN_KIND_BF_COMMUTES (0x40UL << (5 * 8)) /* Operation has the commutative property */
#define UMN_KIND_BF_UNARY (0x20UL << (5 * 8))

/* define a macro that would allow the usage of the stuff as integer constants */
#define UMN_KIND_CREATE_DATA(DATA) ((size_t)(DATA) << (7 * 8))
#define UMN_KIND_CREATE_KWORD(DATA) (UMN_KIND_CREATE_DATA(DATA) | UMN_KIND_BF_LITERAL | UMN_KIND_BF_KEYWORD)

#define UMN_KIND_CREATE_OPERATOR(DATA, PREC_LEVEL) (UMN_KIND_CREATE_DATA(((DATA) << 3) | ((PREC_LEVEL) & 0x07)) | UMN_KIND_BF_LITERAL | UMN_KIND_BF_KEYWORD | UMN_KIND_BF_OPERATOR)
#define UMN_KIND_OPERATOR_PREC_LEVEl(KIND) ((char)((KIND) >> (7 * 8)) & (0x07)) /* the needed data is in the bottom 3 bits*/

/* define constant UMN_KIND_**/
#define UMN_KIND_ERROR UMN_KIND_BF_ERROR
#define UMN_KIND_EOF UMN_KIND_BF_EOF
#define UMN_KIND_INTEGER UMN_KIND_BF_NUMERIC | UMN_KIND_BF_INTEGER
#define UMN_KIND_FRACTION UMN_KIND_BF_NUMERIC | UMN_KIND_BF_FRACTION
#define UMN_KIND_LITERAL UMN_KIND_BF_LITERAL

#define UMN_KIND_VARIABLE UMN_KIND_BF_LITERAL | UMN_KIND_BF_VARIABLE

/* keywords are just unique literals with no no real meaning */
#define UMN_KIND_EQUALS UMN_KIND_CREATE_KWORD(6)
#define UMN_KIND_OBRACKET UMN_KIND_CREATE_KWORD(7) /* Opening bracket "(" */
#define UMN_KIND_CBRACKET UMN_KIND_CREATE_KWORD(8) /* Closing bracket ")" */

/* for simplicity just do the straight forward thing, same as i have done previously */

#define UMN_KIND_ADD UMN_KIND_CREATE_OPERATOR(1, 1) | UMN_KIND_BF_UNARY /* Unary indicates the operator could be unary */ | UMN_KIND_BF_COMMUTES
#define UMN_KIND_SUB UMN_KIND_CREATE_OPERATOR(2, 1) | UMN_KIND_BF_UNARY
#define UMN_KIND_DIV UMN_KIND_CREATE_OPERATOR(3, 2)
#define UMN_KIND_MULT UMN_KIND_CREATE_OPERATOR(4, 2) | UMN_KIND_BF_COMMUTES
#define UMN_KIND_EXP UMN_KIND_CREATE_OPERATOR(5, 4)
/* TODO: figure out the special combination of bit flags to indicate that the following  are unary operators*/
/* TODO:
    Operator data is some magic bs i do not know yet what
*/

/* for keywords, where the data field differentiates the keywords */
static inline int umn_kind_compare(UMN_Kind kind1, UMN_Kind kind2)
{
    return kind1 == kind2;
}

/* for keywords, where the data field differentiates the keywords */
#define umn_kind_is(KIND, COMP_KIND) (!!((size_t)((KIND) & (COMP_KIND))))
#define umn_kind_is_keyword(KIND) umn_kind_is(KIND, UMN_KIND_BF_KEYWORD)
#define umn_kind_is_operator(KIND) umn_kind_is(KIND, UMN_KIND_BF_OPERATOR)
/* END OF KIND MAGIC */

struct UMN_Token
{
    UMN_Kind kind;      /* The token kind */
    __int64_t value[2]; /* When it comes to reading cast it to whatever is required */

    size_t begin; /*store the beginning of the value */
    size_t end;   /*store the position of the last character plus 1*/
};

struct UMN_Keyword
{
    const UMN_Kind kind;
    const unsigned int size;
    const __uint8_t name[16];
};

#define UMN_KWORD_COUNT 8
#define UMN_CREATE_KWORD(KIND, NAME) {.kind = (KIND), .size = sizeof(NAME) - 1 /* remove null byte from size */, .name = NAME}

static struct UMN_Keyword UMN_keywords[] = {
    UMN_CREATE_KWORD(UMN_KIND_ADD, "+"),
    UMN_CREATE_KWORD(UMN_KIND_SUB, "-"),
    UMN_CREATE_KWORD(UMN_KIND_MULT, "*"),
    UMN_CREATE_KWORD(UMN_KIND_DIV, "/"),
    UMN_CREATE_KWORD(UMN_KIND_EXP, "**"),

    UMN_CREATE_KWORD(UMN_KIND_OBRACKET, "("),
    UMN_CREATE_KWORD(UMN_KIND_CBRACKET, ")"),
    UMN_CREATE_KWORD(UMN_KIND_EQUALS, "="),
};

struct UMN_tokeniser
{
    __uint8_t *data;
    size_t data_length;

    size_t position;

    /* keywords stuff */
    size_t keywords_count;
    struct UMN_Keyword *keywords;
};

size_t __power(size_t base, size_t exponent)
{
    size_t v = 1;
    for (size_t i = 0; i < exponent; i++)
    {
        v *= base;
    }
    return v;
}

/* reads from the begin point, if a keyword is found returns the length of the keyword */
size_t match_keyword(struct UMN_tokeniser *t, const size_t begin, struct UMN_Keyword *kword)
{
    if (t == NULL || t->keywords == NULL)
    {
        return 0;
    }

    if (kword == NULL)
    {
        struct UMN_Keyword kw = {0};
        kword = &kw;
    }

    /* find the longest matching keyword */
    int best_match = -1;
    struct UMN_Keyword *curr_kword;

    for (size_t i = 0; i < t->keywords_count; i++)
    {
        curr_kword = &t->keywords[i];

        /* sanity check on name pointer && check the first character of the kword  && check that the keyword size fits into data*/
        if (/* curr_kword == NULL || */ curr_kword->name[0] != t->data[begin] || (begin + curr_kword->size) > t->data_length)
        {
            continue;
        }

        /* match the keyword, first character already checked */
        size_t j = 1;
        for (; (begin + j) < t->data_length && curr_kword->name[j] != '\0' && t->data[begin + j] != '\0'; j++)
        {
            /* check if character mathces the keyword */
            if (isspace(t->data[(begin + j)]) || t->data[(begin + j)] != curr_kword->name[j])
            {
                break;
            }
        }

        /* check if the end was reached */
        if (j < curr_kword->size || curr_kword->name[j] != '\0')
        {
            continue;
        }

        /* check if this is the longest match */
        if (best_match < 0 || (j > t->keywords[best_match].size && curr_kword->size > t->keywords[best_match].size))
        {
            best_match = i;
        }
    }

    if (best_match < 0)
    {
        return 0;
    }

    memcpy(kword, &t->keywords[best_match], sizeof(struct UMN_Keyword));

    return kword->size;
}

size_t atoin(__uint8_t *s, size_t n)
{
    size_t value = 0;

    for (size_t i = 0; i < n; i++)
    {
        if (!isdigit(s[i]))
        {
            return -i;
        }
        value += (s[i] & 0xF) * __power(10, (n - 1) - i);
    }

    return value;
}

/*
    Reads integers by default, and fractions TBD,
    Compares literals to keywords, otherwise greedily reads the entire word
*/
struct UMN_Token
umn_tokeniser_get(struct UMN_tokeniser *t)
{
    struct UMN_Token token = (struct UMN_Token){0};
    __uint8_t curr = t->data[t->position];

    /* consume and read the next token */
    /* Learn about some utf-8 stuff but for now only understand ASCII characters*/

    /* increment position ignoring whitespace and other junk*/
    while (curr <= ' ' && (t->position) < t->data_length) /* off by 1 problem could be here when incrementing position */
    {
        curr = t->data[++t->position];
    }

    if (t->position >= t->data_length)
    {
        return (struct UMN_Token){.kind = UMN_KIND_EOF};
    }

    /* set save the tokens beggining */
    token.begin = t->position++;

    if (isdigit(curr) || (curr == '.' && token.begin + 1 < t->data_length && isdigit(t->data[token.begin + 1]))) /* Read Numeric values*/
    {
        int decimal_dot_pos = -1;
        size_t str_encoding = 10; /* 2(binary), 8(octal)* 10(decimal), 100(decimal dot ...), 16(hexadecimal)*/
        /* Check if the next charcater is on of the spcecial ones */
        token.kind = UMN_KIND_INTEGER;
        token.end = token.begin + 1;

        if (curr == '.')
        {
            decimal_dot_pos = 0;
            str_encoding = 100;
            token.kind = UMN_KIND_FRACTION;
        }

        /* Check for how the number is encoded as a string 0x, 0b ... 100 .* 0.1*/
        if (t->data[token.begin] == '0' && token.begin + 2 < t->data_length && !isspace(t->data[token.begin + 1]) && isxdigit(t->data[token.begin + 2]))
        {
            curr = t->data[token.begin + 1];

            if ((curr | (32)) == 'b')
            {
                token.end = token.begin + 2;
                str_encoding = 2;
            }
            else if ((curr | (32)) == 'x')
            {
                token.end = token.begin + 2;
                str_encoding = 16;
            }
            else if ((curr | (32)) == 'o' || (isdigit(curr) && (curr & 8) == 0)) /* octal */
            {
                token.end = token.begin + 2;
                str_encoding = 8;
            }
            else if (isdigit(curr))
            {
                str_encoding = 10;
            }
            else if (curr == '.')
            {
                str_encoding = 100;
                decimal_dot_pos = 1;
                token.kind = UMN_KIND_FRACTION;

                token.end = token.begin + 2;
            }
            else
            {
                // goto str_to_int;

                /* move the token end to the end of the word */
                for (token.end = token.begin + 1; token.end < t->data_length; token.end++)
                {
                    if (isspace(t->data[token.end]))
                    {
                        break;
                    }
                }

                return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.end};
            }
        }

        /* walk characters break on a bad character*/
        for (; token.end < t->data_length; token.end++)
        {
            curr = t->data[token.end];

            /* validate the current character */
            switch (str_encoding)
            {
            case 2:
                if (!(curr == '1' || curr == '0'))
                {
                    goto str_to_int; /* fail */
                }
                break;
            case 8:
                if (!isdigit(curr) || (curr & 0xF) > 7)
                {
                    goto str_to_int; /* fail */
                }
                break;
            case 16:
                if (!isxdigit(curr))
                {
                    goto str_to_int; /* fail */
                }
                break;
            case 10:
            default:
                /* TODO: handel decimal dot notation */

                if (umn_kind_compare(token.kind, UMN_KIND_INTEGER) && curr == '.')
                {
                    token.kind = UMN_KIND_FRACTION; /* str_to_int does the heavy lifting */
                    str_encoding = 100;             /* */
                    decimal_dot_pos = token.end - token.begin;
                    /* figure out how the decimal is going to be stored*/
                }
                else if (!isdigit(curr))
                {
                    goto str_to_int; /* fail */
                }
            }
        }
    str_to_int:
        /* move tokeniser position */
        t->position = token.end;
        int offset;
        switch (str_encoding)
        {
        case 2: /* convert 0b1010 to 10*/
            /* magic numbers exist here due the fact that the hex string is preceeded by 0b*/
            offset = (token.end - token.begin - 3);
            /* this is jank count remove underscores from the offset */

            for (size_t i = 0; i < (token.end - token.begin - 2); i++)
            {
                curr = t->data[token.begin + 2 + i];

                if (curr == '0')
                {
                    ; /* noop*/
                }
                else if (curr == '1')
                {
                    token.value[0] |= (1 << offset--);
                }
                else
                {
                    return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.begin + 2 + i};
                }
            }
            break;
        case 8: /* convert 0o12 to 10*/
            /* magic numbers exist here due the fact that the hex string is preceeded by 0o*/
            int magic = 2;
            if (isdigit(t->data[token.begin + 1]))
            { /* support 0123 , octal representation*/
                magic = 1;
            }

            offset = (token.end - token.begin - magic - 1);
            /* this is jank count remove underscores from the offset */

            for (size_t i = 0; i < (token.end - token.begin - magic); i++)
            {
                curr = t->data[token.begin + magic + i];

                if (isdigit(curr) && (curr & 8) == 0)
                {
                    token.value[0] |= (size_t)(curr & 0x7) << (3 * offset--);
                }
                else
                {
                    return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.begin + magic + i};
                }
            }
            break;
        case 16: /* convert 0x0a to 10 */
                 /* magic numbers exist here due the fact that the hex string is preceeded by 0x*/
            offset = (token.end - token.begin - 3);
            for (size_t i = 0; i < (token.end - token.begin - 2); i++)
            {
                curr = t->data[token.begin + 2 + i];

                if (isdigit(curr))
                {
                    token.value[0] |= (size_t)(curr & 0xF) << (4 * offset--);
                }
                else if (curr | (32) >= 'a' && curr | (32) <= 'f')
                {
                    token.value[0] |= (size_t)((curr & ~(32)) - 'A' + 10) << (4 * offset--);
                }
                else
                {
                    return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.begin + 2 + i};
                }
            }
            break;
        case 100:
        {
            if (decimal_dot_pos < 0)
            {
                return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.end};
            }

            int fraction_part_length = token.end - token.begin - decimal_dot_pos - 1;
            size_t integer_part = atoin((t->data + token.begin), decimal_dot_pos);
            size_t fraction_part = atoin((t->data + token.begin + decimal_dot_pos + 1), fraction_part_length);

            size_t denominator = __power(10, fraction_part_length);
            token.value[0] = integer_part * denominator + fraction_part;
            token.value[1] = denominator;
        };
        break;
        case 10:
        default: /* convert '10' to 10 */
            token.value[0] = atoin((t->data + token.begin), (token.end - token.begin));
            if (token.value[0] < 0)
            {
                return (struct UMN_Token){.kind = UMN_KIND_ERROR, .begin = token.begin, .end = token.begin + (-1 * (token.value[0]))};
            }
        }

        if (umn_kind_compare(token.kind, UMN_KIND_INTEGER))
        {
            token.value[1] = str_encoding;
        }

        return token;
    }
    else /* Read keywords and literals */
    {
        /* check if the current position matches a keyword */
        struct UMN_Keyword kword = {0};

        if (match_keyword(t, token.begin, &kword))
        {
            token.end = token.begin + kword.size;
            t->position = token.end;

            token.kind = kword.kind;

            if (kword.size > (sizeof(token.value) - 1))
            {
                fprintf(stderr, "umn_tokeniser_get: keyword too big\n");
                return (struct UMN_Token){.kind = UMN_KIND_ERROR};
            }

            /* write keyword name into token value */
            memcpy((__uint8_t *)&token.value, kword.name, kword.size);

            return token;
        }

        /* no keyword match, now get values until space or keyword */
        token.end = token.begin + 1;
        for (; token.end < t->data_length; token.end++)
        {
            if (isspace(t->data[token.end]) || isdigit(t->data[token.end]) || match_keyword(t, token.end, NULL))
            {
                break;
            };
        }

        t->position = token.end;
        token.kind = UMN_KIND_LITERAL;

        if ((token.end - token.begin) > sizeof(token.value) - 1)
        {
            fprintf(stderr, "umn_tokeniser_get: keyword too big\n");
            return (struct UMN_Token){.kind = UMN_KIND_ERROR};
        }

        /* write literal into token value */
        memcpy((__uint8_t *)&token.value, &t->data[token.begin], (token.end - token.begin));

        return token;
    }

    return (struct UMN_Token){.kind = UMN_KIND_ERROR};
}

/* include an arena where i can keep stuff */
#include "umn-arena.h"

static inline void umn_token_print(struct UMN_Token token)
{
    /* token kind to string */
    char *s = "\0"; /* empty null string */
    switch (token.kind)
    {
    case UMN_KIND_ERROR:
        s = "UMN_KIND_ERROR";
        break;
    case UMN_KIND_EOF:
        s = "UMN_KIND_EOF";
        break;
    case UMN_KIND_INTEGER:
        s = "UMN_KIND_INTEGER";
        break;
    case UMN_KIND_FRACTION:
        s = "UMN_KIND_FRACTION";
        break;
    case UMN_KIND_LITERAL:
        s = "UMN_KIND_LITERAL";
        break;
    case UMN_KIND_ADD:
        s = "UMN_KIND_ADD";
        break;
    case UMN_KIND_SUB:
        s = "UMN_KIND_SUB";
        break;
    case UMN_KIND_DIV:
        s = "UMN_KIND_DIV";
        break;
    case UMN_KIND_MULT:
        s = "UMN_KIND_MULT";
        break;
    case UMN_KIND_EXP:
        s = "UMN_KIND_EXP";
        break;
    case UMN_KIND_EQUALS:
        s = "UMN_KIND_EQUALS";
        break;
    case UMN_KIND_OBRACKET:
        s = "UMN_KIND_OBRACKET";
        break;
    case UMN_KIND_CBRACKET:
        s = "UMN_KIND_CBRACKET";
        break;
    }

    if (umn_kind_compare(token.kind, UMN_KIND_INTEGER))
    {
        printf("Token.kind = %s, Token.value_int = %ld, Token.begin = %zu, Token.end = %zu\n", s, token.value[0], token.begin, token.end);
    }
    else if (umn_kind_compare(token.kind, UMN_KIND_FRACTION))
    {
        printf("Token.kind = %s, Token.value_float = %f, Token.begin = %zu, Token.end = %zu\n", s, (double)token.value[0] / token.value[1], token.begin, token.end);
    }
    else
    {
        printf("Token.kind = %s, Token.value_str = \'%s\', Token.begin = %zu, Token.end = %zu\n", s, (char *)token.value, token.begin, token.end);
    }
}

void umn_parse_print_error(struct UMN_tokeniser *tokeniser, struct UMN_Token token)
{
    static __uint8_t msg_buffer[80]; /* this could be static so that it could be modified and stuff */

    if (tokeniser->data_length >= sizeof(msg_buffer))
    {
        /* unhandled do some kind of processing IDK */
        printf("umn_parse: failed to output error message, at (%zu -> %zu)\n", token.begin, token.end);
        return;
    }
    else
    {
        assert(tokeniser->data[tokeniser->data_length] == '\0');

        fputs(tokeniser->data, stderr); /* assume there is a null byte */
        fputc('\n', stderr);            /* write new line */

        for (size_t i = 0; i < tokeniser->data_length; i++)
        {
            fputc(
                (i == token.begin) ? '^' : (i > token.begin && i < token.end) ? '~'
                                                                              : ' ',
                stderr);
        }

        fputc('\n', stderr); /* write new line */

        /* TODO: get a error message from somewhere ...*/
        fputs("Something went wrong with the tokeniser \n", stderr);
    }
}

struct UMN_PNode
{
    struct UMN_Token token;
    /* There should be some metadata as to if what the nodes relation to others is*/

    // struct UMN_PNode *parent; /* a reference to the nodes parent */
    size_t child_count;
    struct UMN_PNode *children; /* this would be a pointer to an array of nodes that are children */
};

int umn_parse_node_to_string(__uint8_t *s, size_t max_len, struct UMN_PNode *pnode, int wrap /* wheter node should be wrapped within brackets*/)
{
    int n = 0;
    if (wrap && (max_len - n) > 1)
    {
        s[n++] = '(';
    }

    if (umn_kind_is(pnode->token.kind, UMN_KIND_BF_INTEGER))
    {
        /* keep in mind the different encoding available */
        n += snprintf(s + n, max_len - n, "%ld", pnode->token.value[0]);
    }
    else if (umn_kind_is(pnode->token.kind, UMN_KIND_BF_FRACTION))
    {
        n += snprintf(s + n, max_len - n, "%f", (double)pnode->token.value[0] / pnode->token.value[1]);
    }
    else if (!umn_kind_is(pnode->token.kind, UMN_KIND_LITERAL))
    {
        /* not a literal so nothing */
        n += (*s = 0);
    }
    else if (pnode->child_count == 0)
    {
        n += snprintf(s + n, max_len - n, "%s", (char *)(pnode->token.value));
    }
    else
    {
        assert(pnode->children);

        if (pnode->child_count == 1)
        {
            n += snprintf(s + n, max_len - n, "%s", (char *)(pnode->token.value));
            n += umn_parse_node_to_string(s + n, max_len - n, pnode->children, pnode->children->child_count);
        }
        else
        {
            for (size_t i = 0; i < pnode->child_count && (max_len - n) > 1; i++)
            {
                if (i > 0)
                {
                    s[n++] = ' ';

                    n += snprintf(s + n, max_len - n, "%s", (char *)(pnode->token.value));
                    if ((max_len - n) < 0)
                    {
                        return n;
                    }
                    s[n++] = ' ';
                }

                /* if child is an operator with an inferior operator precedence level */
                int should_wrap = (umn_kind_is(((pnode->children + i)->token.kind), UMN_KIND_BF_OPERATOR)) && UMN_KIND_OPERATOR_PREC_LEVEl((pnode->children + i)->token.kind) <= UMN_KIND_OPERATOR_PREC_LEVEl(pnode->token.kind);
                n += umn_parse_node_to_string(s + n, max_len - n, pnode->children + i, should_wrap);
            }
        }
    }

    if (wrap && (max_len - n) > 1)
    {
        s[n++] = ')';
    }

    return n;
}

void umn_parse_node_print(struct UMN_PNode *pnode)
{
    char message_buffer[100] = {0};

    umn_parse_node_to_string(message_buffer, 99, pnode, 0);
    puts(message_buffer);
}

void umn_parse(__uint8_t *data)
{
    puts(data);
    putchar('\n');

    struct UMN_tokeniser tokeniser = {
        .data = data,
        .data_length = strlen(data),

        .position = 0,

        .keywords = UMN_keywords,
        .keywords_count = UMN_KWORD_COUNT,
    };

    /* initialse the arena */
    struct UMN_Arena *arena = umn_arena_init(sizeof(struct UMN_PNode) * 200);
    if (arena == NULL)
    {
        fputs("umn_parse: failed to initialise the memory arena\n", stderr);
        goto error_bad;
    }

    /* the stack was an idea but let's just use an array */
    /* instead of node list use a stack */
    struct UMN_Stack *node_stack = umn_stack_init(arena, 50 /* have the space for 30 elements */, sizeof(struct UMN_PNode));
    if (node_stack == NULL)
    {
        fputs("umn_parse: failed to initialise node stack\n", stderr);
        goto error_bad;
    }

    struct UMN_Stack *child_stack = umn_stack_init(arena, 10, sizeof(struct UMN_PNode));
    assert(child_stack != NULL);

    struct UMN_PNode pnode = {};

    /* approach from <https://github.com/Snorkungen/expression/blob/master/expression_tree_builder2.py> */

    /* collect tokens into a node list */

    do /* just read all nodes and push*/
    {
        pnode.token = umn_tokeniser_get(&tokeniser);

        /* Handle an errors from tokeniser */
        if (umn_kind_compare(pnode.token.kind, UMN_KIND_ERROR))
        {
            /* debug print the surounding characters on a line */
            umn_parse_print_error(&tokeniser, pnode.token);
            fputs("umn_parse: tokeniser failed to read a token erroneus input\n", stderr);
            goto error_bad;
        }
        else if (umn_kind_compare(pnode.token.kind, UMN_KIND_EOF))
        {
            break;
        }

        // (umn_stack_peek(node_stack, &prev_node, 1) /* true if failed to read previous node */ || umn_kind_is_operator(prev_node.token.kind))

        /* push node onto stack */
        if (umn_stack_push(node_stack, &pnode, NULL))
        {
            /* handle error failed to push node onto stack */

            /* TODO: stack has probably reached it's capacity and needs to be expanded */

            fputs("umn_parse: failed to push node onto stack\n", stderr);
            goto error_bad;
        }
    } while (umn_kind_compare(pnode.token.kind, UMN_KIND_EOF) == 0);

    { /* do actual logic interpretting tokens and build a tree of the node-tokens*/
        struct UMN_PNode *curr = NULL, *prev, *next;

        /* this might not be the best way but i think it is cool */
        /* have stack of (base index) and (end index) */
        struct _base_end
        {
            size_t base;
            size_t end;
            size_t set_end; /* used to calculate the stack has been shifted by */
        };

        struct UMN_Stack *be_stack = umn_stack_init(arena, 12 /* max depth */, sizeof(struct _base_end));

        size_t base = 0;

        char oper_prec_level = 6; /* [brackets are handled seperately] unary_operators(3) functions(5), exponentiation(4), multiply/division(2) addition/subtraction(1) */
        do                        /* do stuff */
        {
            size_t i;
            for (i = base; i < node_stack->index; i++)
            {
                curr = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i));
                prev = (i > base) ? (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (i - 1))) : NULL;
                next = ((i + 1) < node_stack->index) ? (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (i + 1)))
                                                     : NULL;

                /* before everything else deal with brackets ...*/
                if (umn_kind_compare(curr->token.kind, UMN_KIND_OBRACKET) && oper_prec_level == 6) /* test for opening bracket */
                {
                    /* somehow consume until it finds a closing bracket */
                    /* recursion is not what i want to do ...*/

                    /* TODO: do some error handling of this garbage */
                    int depth = 1;
                    size_t j;

                    for (j = i + 1; j < node_stack->index; j++)
                    {
                        UMN_Kind kind = ((struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (j))))->token.kind;

                        if (umn_kind_compare(kind, UMN_KIND_OBRACKET))
                        {
                            depth += 1;
                        }
                        else if (umn_kind_compare(kind, UMN_KIND_CBRACKET))
                        {
                            depth -= 1;
                        }

                        if (depth == 0)
                        {
                            break;
                        }
                    }

                    /* TODO: do some error handling */
                    assert(depth == 0); /**/

                    /* first move the bytes from i + 1 shift by 1 */

                    memmove(/* shift the elements within the brackets back by one node */
                            node_stack->data + (i)*node_stack->element_size,
                            node_stack->data + ((i + 1) * node_stack->element_size),
                            (node_stack->element_capacity * node_stack->element_size) - ((i + 1) * node_stack->element_size));

                    j -= 1;

                    memmove(/* shift the elements beyond the brackets back by one node */
                            node_stack->data + (j)*node_stack->element_size,
                            node_stack->data + ((j + 1) * node_stack->element_size),
                            (node_stack->element_capacity * node_stack->element_size) - ((j + 1) * node_stack->element_size));

                    node_stack->index -= 2; /* 2 elements were removed  */

                    umn_stack_push(be_stack, &((struct _base_end){.base = base, .end = node_stack->index, .set_end = j}), NULL);
                    node_stack->index = j;
                    base = i;

                    i -= 1;
                    /* delete the brackets tokens from existance */
                    continue;
                }

                /* lazy hack */
                if (oper_prec_level == 6)
                {
                    continue;
                }

                /* for testing only care about operators */
                if (!umn_kind_is_operator(curr->token.kind))
                {
                    continue; /* only operators have a use */
                }

                if (next == NULL)
                {
                    if (curr->child_count > 0)
                    {
                        continue;
                    }
                    umn_parse_print_error(&tokeniser, curr->token);
                    fputs("umn_parse: token expected\n", stderr);
                    goto error_bad;
                }

                if (curr->child_count > 0 && umn_kind_is(next->token.kind, UMN_KIND_BF_OPERATOR))
                {
                    continue;
                }

                /* UNARY Hanlding, +-+-(a) ...   */
                /* TODO: support nesting unary operators */
                /* detect situation where being a unary operator is suitable */
                if (umn_kind_is(curr->token.kind, UMN_KIND_BF_UNARY) && oper_prec_level == 3 /* highest operator precedence level */)
                {
                    /* pre reqs to treat operator as unary
                        - first operator - 1
                        - previous token is an unitialised operator 2 (* - 1)
                     */
                    if (prev == NULL || (umn_kind_is(prev->token.kind, UMN_KIND_BF_OPERATOR) && prev->child_count < 2))
                    {
                        /* handle unary '+' & '-' */
                        /* check something according to some types of rules and do stuff skip for now */

                        /* count how many minus and pluses there */
                        int offset = 1;
                        int minus_count = umn_kind_compare(curr->token.kind, UMN_KIND_SUB) ? 1 : 0;

                        for (; (i + offset) < node_stack->index; offset++)
                        {
                            next = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (i + offset)));

                            if (!umn_kind_is(next->token.kind, UMN_KIND_BF_UNARY) || next->child_count > 0)
                            {
                                break;
                            }

                            minus_count += umn_kind_compare(next->token.kind, UMN_KIND_SUB) ? 1 : 0;
                        }

                        /* check that the next token is not an unitialised operator */
                        if (umn_kind_is(next->token.kind, UMN_KIND_BF_OPERATOR) && next->child_count == 0)
                        {
                            umn_parse_print_error(&tokeniser, (struct UMN_Token){.kind = curr->token.kind, .begin = curr->token.begin, .end = next->token.end}); /* bad stuff */
                            fputs("umn_parse: next token is invalid \n", stderr);
                            goto error_bad;
                        }

                        /*
                            BELOW COMMENTED SECTION IS MULTIPLYING VALUE BY -1
                        */

                        // if ((minus_count & 1) == 1)
                        // {
                        //     if (umn_kind_is(next->token.kind, UMN_KIND_BF_NUMERIC))
                        //     {
                        //         next->token.value[0] *= -1; /* make value negative */
                        //     }
                        //     else
                        //     {
                        //         /* create a new node that is a multiplication of -1 * (next) */
                        //         struct UMN_PNode *children = umn_arena_alloc(arena, sizeof(struct UMN_PNode) * 2);

                        //         children->token.kind = UMN_KIND_INTEGER;
                        //         children->token.value[0] = -1;

                        //         memcpy(children + 1, next, sizeof(struct UMN_PNode)); /* copy the next value into the the next child */

                        //         struct UMN_PNode mult_node = {
                        //             .token = {.kind = UMN_KIND_MULT, .value = {(size_t)'*' /* this works due to little endian */, 0}, .begin = curr->token.begin, .end = next->token.end},
                        //             .child_count = 2,
                        //             .children = children};

                        //         memcpy(next, &mult_node, sizeof(struct UMN_PNode));
                        //     }
                        // }

                        // /* delete the first node and  shift stack back down */
                        // memmove(node_stack->data + node_stack->element_size * i, node_stack->data + node_stack->element_size * (i + offset),
                        //         (node_stack->element_capacity * node_stack->element_size) - ((i + offset) * node_stack->element_size));

                        // node_stack->index -= offset;

                        /*
                            BELOW CREATE A UNARY OPERATION BY WRAPPING RESULTING TOKEN
                        */

                        { /* THIS IS DOING WHAT I WANTED TO AVOID CREATING INFORMATION */ 
                            curr->token.kind = (minus_count & 1) == 1 ? UMN_KIND_SUB : UMN_KIND_ADD;                                                                                               /* modify token kind */
                            curr->token.value[0] = (minus_count & 1) == 1 ? (size_t)'-' | ((size_t)'-' << ((sizeof(size_t) - 1) * 8)) : (size_t)'+' | ((size_t)'+' << ((sizeof(size_t) - 1) * 8)); /*  */
                        }

                        curr->child_count = 1;
                        curr->children = umn_arena_alloc(arena, sizeof(struct UMN_PNode));
                        memmove(curr->children, next, sizeof(struct UMN_PNode));

                        /* remove the element immediatly after the series of unary operators */
                        memmove(node_stack->data + node_stack->element_size * (i + 1), node_stack->data + node_stack->element_size * (i + offset + 1),
                                (node_stack->element_capacity * node_stack->element_size) - ((i + offset) * node_stack->element_size));
                        node_stack->index -= offset;

                        continue;
                    }
                }

                /* now comes the hard part deal with operator precedence */
                if (oper_prec_level > UMN_KIND_OPERATOR_PREC_LEVEl(curr->token.kind))
                {
                    continue; /* touch this on the next pass */
                }

                if (prev == NULL)
                {
                    umn_parse_print_error(&tokeniser, curr->token);
                    fputs("umn_parse: expected token before\n", stderr);

                    // for (size_t i = 0; i < node_stack->index; i++)
                    // {
                    //     umn_parse_node_print(
                    //         (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)));
                    //     // umn_token_print(
                    //     //     ((struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)))->token);
                    // }
                    goto error_bad;
                }

                /* small loop to collect children */
                int offset_amount = 0;
                child_stack->index = 0;
                if (prev != NULL)
                {
                    offset_amount++; /* this is before the current index */
                    assert(umn_stack_push(child_stack, prev, arena) == 0);
                }

                if (umn_kind_is(curr->token.kind, UMN_KIND_BF_COMMUTES))
                { /* a known situation fix 1 * (-1 * a) */
                    // puts("**** THERE ARE ASSUMPTIONS HERE THAT DO NOT WORK ****");

                    /* The logic is such that if the previous node is the same operation as the previous append the node */

                    for (size_t j = i + 1; j < node_stack->index; j++)
                    {
                        next = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * j));

                        /* if the node is not an operation i.e variable or integer or either ..*/

                        /* the thing that would allow a continue is an unitiialised operator of the same type */
                        if (umn_kind_compare(curr->token.kind, next->token.kind))
                        { /* the node is the same so a continue would be allowed otherwise break */
                            offset_amount++;

                            if (next->child_count >= 2)
                            { /* loop through the children of the node */
                                for (size_t k = 0; k < next->child_count; k++)
                                {
                                    assert(umn_stack_push(child_stack, (next->children + k), arena) == 0);
                                }
                            }
                        }
                        else if (umn_kind_is(next->token.kind, UMN_KIND_BF_OPERATOR) && next->child_count == 0)
                        {
                            break; /* quit ecountered an uninitialised operater of a different kind */
                        }
                        else
                        {
                            assert(umn_stack_push(child_stack, next, arena) == 0);
                            offset_amount++;
                        }
                    }
                }
                else
                {
                    assert(umn_stack_push(child_stack, next, arena) == 0);
                    offset_amount++;
                }

                curr->child_count = child_stack->index;
                curr->children = umn_arena_alloc(arena, sizeof(struct UMN_PNode) * curr->child_count);

                { /* modify stack by removing children from stack and shifting nodes backwards */
                    int elements_to_shift_by = offset_amount;

                    // printf("stack index = %zu, offset_amount = %d\n", node_stack->index, offset_amount);
                    // printf("child count = %zu\n", child_stack->index);

                    memcpy(curr->children, child_stack->data, sizeof(struct UMN_PNode) * curr->child_count);

                    /* Do some array modificiations so the stack is happy */

                    /* check if there are bytes to copy back */

                    memcpy(&pnode, curr, (sizeof(struct UMN_PNode)));

                    memmove(
                        node_stack->data + (i)*node_stack->element_size,
                        node_stack->data + ((i + elements_to_shift_by) * node_stack->element_size),
                        (node_stack->element_capacity * node_stack->element_size) - ((i + elements_to_shift_by) * node_stack->element_size));

                    /* move stack index back by 2 */

                    node_stack->index -= elements_to_shift_by;

                    memcpy(node_stack->data + (i - 1) * node_stack->element_size, &pnode, (sizeof(struct UMN_PNode)));
                }

                // umn_parse_node_print(&pnode);
                // printf("stack index = %zu\n", node_stack->index);

                i -= 1; /* move index back to rectify that the stack has been m modified */
            }

            if (oper_prec_level == 1 && be_stack->index)
            {
                struct _base_end be = {0};
                assert(umn_stack_pop(be_stack, &be) == 0); /* pop from be_stack */

                /* set values this is going to off due to the fact that the brackets exist and take up space */
                base = be.base;
                node_stack->index = be.end - (be.set_end - node_stack->index);

                oper_prec_level = 6 + 1;
            }

        } while (--oper_prec_level > 0);
    }

    putchar('\n');
    for (size_t i = 0; i < node_stack->index; i++)
    {
        umn_parse_node_print(
            (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)));
        umn_token_print(
            ((struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)))->token);
    }

    /* handle edge case */
    if (node_stack->index == 1)
    {
        /* Quit nothing to do ?? */
    }
    else if (node_stack->index == 0)
    {
        goto error_bad;
    }

    /* finally delete the arena */
    umn_arena_delete(arena);
    return;

error_bad:
    umn_arena_delete(arena);
    return;
}

#endif /* UMN_H */