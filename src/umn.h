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
*/

#ifndef UMN_H
#define UMN_H

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

/* BEGINING OF KIND MAGIC */
typedef union
{
    size_t full; /* allow easier comparison */
    struct
    {
        __uint8_t data; /* 8-bit field where kind specific information get's stored */

        __uint8_t is_error : (1);   /* if the token is an error */
        __uint8_t is_eof : (1);     /* the token is the last token in the tokeniser datas */
        __uint8_t is_numeric : (1); /* if the number is either a INTEGER or FRACTION */
        __uint8_t is_literal : (1);

        __uint8_t is_integer : (1);
        __uint8_t is_fraction : (1);
        __uint8_t is_operator : (1);
    };
} UMN_Kind;

/* compare if the values are the same, ignoring data fields */
static inline int umn_kind_compare(UMN_Kind kind1, UMN_Kind kind2)
{
    return (kind1.full >> (sizeof(kind1.data) * 8)) == (kind2.full >> (sizeof(kind2.data) * 8));
}

const static UMN_Kind UMN_KIND_ERROR = {.is_error = 1};
const static UMN_Kind UMN_KIND_EOF = {.is_eof = 1};
const static UMN_Kind UMN_KIND_INTEGER = {.is_numeric = 1, .is_integer = 1};
const static UMN_Kind UMN_KIND_FRACTION = {.is_numeric = 1, .is_fraction = 1};
const static UMN_Kind UMN_KIND_LITERAL = {.is_literal = 1};

/* predefined keywords operators */
const static UMN_Kind UMN_KIND_PLUS = {.is_literal = 1, .is_operator = 1}; /* how does unary operation work ?? */
const static UMN_Kind UMN_KIND_MINUS = {.is_literal = 1, .is_operator = 1};
const static UMN_Kind UMN_KIND_FSLASH = {.is_literal = 1, .is_operator = 1};
const static UMN_Kind UMN_KIND_STAR = {.is_literal = 1, .is_operator = 1};
const static UMN_Kind UMN_KIND_STAR_STAR = {.is_literal = 1, .is_operator = 1};
const static UMN_Kind UMN_KIND_EQUALS = {.is_literal = 1, .is_operator = 1};

/* predefined  meaningful notation get this to compile */
const static UMN_Kind UMN_KIND_OBRACKET = {.data = 0x70, .is_literal = 1}; /* Opening bracket "(" */
const static UMN_Kind UMN_KIND_CBRACKET = {.data = 0xF0, .is_literal = 1}; /* Opening bracket ")" */

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
#define UMN_CREATE_KWORD(KIND, NAME) {.kind = KIND, .size = sizeof(NAME) - 1 /* remove null byte from size */, .name = NAME}

static struct UMN_Keyword UMN_keywords[] = {
    UMN_CREATE_KWORD(UMN_KIND_PLUS, "+"),
    UMN_CREATE_KWORD(UMN_KIND_MINUS, "-"),
    UMN_CREATE_KWORD(UMN_KIND_STAR, "*"),
    UMN_CREATE_KWORD(UMN_KIND_FSLASH, "/"),
    UMN_CREATE_KWORD(UMN_KIND_STAR_STAR, "**"),
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

struct UMN_Token
umn_tokeniser_peek(struct UMN_tokeniser *t)
{
    size_t position = t->position;
    struct UMN_Token token = umn_tokeniser_get(t);
    t->position = position;
    return token;
}

/* include an arena where i can keep stuff */
#include "umn-arena.h"

static inline void umn_token_print(struct UMN_Token token)
{
    if (umn_kind_compare(token.kind, UMN_KIND_INTEGER))
    {
        printf("Token.kind = %ld, Token.value_int = %ld, Token.begin = %zu, Token.end = %zu\n", token.kind.full, token.value[0], token.begin, token.end);
    }
    else if (umn_kind_compare(token.kind, UMN_KIND_FRACTION))
    {
        printf("Token.kind = %ld, Token.value_float = %f, Token.begin = %zu, Token.end = %zu\n", token.kind.full, (double)token.value[0] / token.value[1], token.begin, token.end);
    }
    else
    {
        printf("Token.kind = %ld, Token.value_str = \'%s\', Token.begin = %zu, Token.end = %zu\n", token.kind.full, (char *)token.value, token.begin, token.end);
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

void umn_parse(__uint8_t *data)
{
    struct UMN_tokeniser tokeniser = {
        .data = data,
        .data_length = strlen(data),

        .position = 0,

        .keywords = UMN_keywords,
        .keywords_count = UMN_KWORD_COUNT,
    };

    /* initialse the arean */
    struct UMN_Arena *arena = umn_arena_init(sizeof(struct UMN_PNode) * 200);
    if (arena == NULL)
    {
        fputs("umn_parse: failed to initialise the memory arena\n", stderr);
        return;
    }

    size_t node_list_capacity = 20, node_list_size = 0;
    struct UMN_PNode *node_list = umn_arena_alloc(arena, sizeof(struct UMN_PNode) * node_list_capacity);
    if (node_list == NULL)
    {
        fputs("umn_parse: failed to initialise the node_list\n", stderr);
        return;
    }

    struct UMN_PNode head = {};

    struct UMN_Token token_prev = {}; /* this might be wasteful initialize an empty token to begin with */
    struct UMN_Token token = {};

    /* approach from <https://github.com/Snorkungen/expression/blob/master/expression_tree_builder2.py> */

    /* collect tokens into a node list */
    while (umn_kind_compare(token.kind, UMN_KIND_EOF) == 0)
    {
        token = umn_tokeniser_get(&tokeniser);

        /* emit information that something has gone wrong when getting token */
        if (umn_kind_compare(token.kind, UMN_KIND_ERROR))
        {
            /* debug print the surounding characters on a line */
            static __uint8_t msg_buffer[80]; /* this could be static so that it could be modified and stuff */

            if (tokeniser.data_length >= sizeof(msg_buffer))
            {
                /* unhandled do some kind of processing IDK */
                printf("umn_parse: failed to output error message, at (%zu -> %zu)\n", token.begin, token.end);
                return;
            }
            else
            {
                assert(tokeniser.data[tokeniser.data_length] == '\0');

                fputs(tokeniser.data, stderr); /* assume there is a null byte */
                fputc('\n', stderr);           /* write new line */

                for (size_t i = 0; i < tokeniser.data_length; i++)
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

            continue;
        }
        else if (umn_kind_compare(token.kind, UMN_KIND_EOF))
        {
            break;
        }

        /* just collect all nodes int an array */
        if (node_list_size >= node_list_capacity)
        {
            node_list_capacity *= 2;
            node_list = umn_arena_realloc(arena, node_list, sizeof(struct UMN_PNode) * node_list_capacity);

            if (node_list == NULL)
            {
                fputs("umn_parse: node_list reached it's capacity\n", stderr);
                return;
            }
        }

        /* append node to list */
        memcpy(&node_list[node_list_size++].token, &token, sizeof(token));
    };

    /* loop through nodes and do stuff */
    for (size_t i = 0; i < node_list_size; i++)
    {

        token = node_list[i].token;
        /* Read token and do something idk what but all right ...*/
        /* What is the  goal to construct an tre structure ??*/

        /* read and emit token */
        umn_token_print(token);
    }

    /* handle edge case */
    if (node_list_size == 1)
    {
        /* Quit nothing to do ?? */
        return;
    }
    else if (node_list_size == 0)
    {
        return; /* IDK */
    }

    struct UMN_PNode *curr_node = NULL, *prev_node = NULL;

    /* this thing needs a stack of operator ??*/

    /* First pass use unary operators ?? */
    for (size_t i = 0; i < node_list_size; i++)
    {
        curr_node = &node_list[i];

        /* Context would describe the action of the token */
        if (umn_kind_compare(curr_node->token.kind, UMN_KIND_PLUS) || umn_kind_compare(curr_node->token.kind, UMN_KIND_MINUS))
        {
            /*
                Is unary operator
                    - first token, in series
                    - previous token is an operator
            */
            if (prev_node == NULL || prev_node->token.kind.is_operator)
            {
                /* the node is a unary operator */
                /* rewrite plus literal to an a POSITIVE kind */

                /* get the next node in the series */

                puts("this is a unary operator");
                if (i + 1 < node_list_size)
                {
                    struct UMN_Token next_token = node_list[i + 1].token;

                    if (curr_node->token.end != next_token.begin)
                    {
                        /* this should be a warning due to you doing some stuff like "+ 2" ?? or " (- a)"*/
                    }

                    /* Dispatch some magic function that modifies the following token and constructs a new node */

                    if (next_token.kind.is_numeric)
                    {
                        next_token.value[0] *= -1;
                    }

                    umn_token_print(next_token);
                }
            }
        }

        prev_node = curr_node;
    }

    /* finally delete the arena */
    umn_arena_delete(arena);
}

#endif /* UMN_H */