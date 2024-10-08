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
#define UMN_KIND_BF_FUNCTION (0x10UL << (5 * 8)) /* the keyword is a function ... */

/* define a macro that would allow the usage of the stuff as integer constants */
#define UMN_KIND_CREATE_DATA(DATA) ((size_t)(DATA) << (7 * 8))
#define UMN_KIND_CREATE_KWORD(DATA) (UMN_KIND_CREATE_DATA(DATA) | UMN_KIND_BF_LITERAL | UMN_KIND_BF_KEYWORD)
#define UMN_KIND_CREATE_FUNC(DATA) (UMN_KIND_CREATE_KWORD(DATA) | UMN_KIND_BF_FUNCTION)

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
#define UMN_KIND_COMMA UMN_KIND_CREATE_KWORD(9)

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

#define UMN_KWORD_COUNT 10
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
    UMN_CREATE_KWORD(UMN_KIND_COMMA, ","),

    UMN_CREATE_KWORD(UMN_KIND_CREATE_FUNC(1), "testf"),
};

struct UMN_Tokeniser
{
    char *data;
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
size_t match_keyword(struct UMN_Tokeniser *t, const size_t begin, struct UMN_Keyword *kword)
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

size_t atoin(char *s, size_t n)
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
umn_tokeniser_get(struct UMN_Tokeniser *t)
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
                    offset--;
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
        {       /* magic numbers exist here due the fact that the hex string is preceeded by 0o*/
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
        }
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
                else if ((curr | (32)) >= 'a' && (curr | (32)) <= 'f')
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

/* write the value of the token to to `s`*/
int umn_token_to_string(struct UMN_Token *token, char *s, size_t max_len)
{
    if (umn_kind_is(token->kind, UMN_KIND_BF_LITERAL))
    {
        return snprintf(s, max_len, "%s", (char *)token->value);
    }

    if (umn_kind_is(token->kind, UMN_KIND_BF_FRACTION))
    {
        return snprintf(s, max_len, "%f", (double)token->value[0] / token->value[1]);
    }

    if (!umn_kind_is(token->kind, UMN_KIND_BF_INTEGER))
    {
        return 0;
    }

    /* respect the encoding in the integer */
    switch (token->value[1])
    {
    case 2: /* binary 0b....*/
    {
        size_t n = 0, value = token->value[0], offset = 0;
        if (n + 2 < max_len)
        {
            s[n++] = '0';
            s[n++] = 'b';
        }

        /* move offset until first set bit is found */
        for (offset = 0; (value & ((0xffULL << 7 * 8)) >> ((offset) * 8)) == 0; offset++)
        {
            ; /* noop the above statement does some cursed B.S. */
        }

        offset = offset * 8; /* multiply byte offset to bits */

        /* move offset untill it finds its first byte with a 1*/
        while (offset <= sizeof(value) * 8)
        {
            if (value & (1ULL << (sizeof(value) * 8 - (++offset))))
            {
                break;
            }
        }

        while (offset <= sizeof(value) * 8 && n + 1 < max_len)
        {
            s[n++] = value & (1ULL << (sizeof(value) * 8 - offset++)) ? '1' : '0';
        }

        return n;
    }
    case 8: /* octal 0o....*/
        return snprintf(s, max_len, "0o%lo", token->value[0]);
    case 16: /* hexadecimal 0x.... */
        return snprintf(s, max_len, "%#lx", token->value[0]);
    case 10:
    default: /* decimal 1234...*/
        return snprintf(s, max_len, "%zu", token->value[0]);
    }
}

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
    case UMN_KIND_COMMA:
        s = "UMN_KIND_COMMA";
        break;
    }

    if (umn_kind_is(token.kind, UMN_KIND_BF_FUNCTION))
    {
        s = "UMN_KIND_FUNCTION";
    }

    char vs[100] = {0};
    umn_token_to_string(&token, vs, 99);
    printf("Token.kind = %s, Token.value = \"%s\", Token.begin = %zu, Token.end = %zu\n", s, vs, token.begin, token.end);
}

void umn_parse_print_error(struct UMN_Tokeniser *tokeniser, struct UMN_Token token)
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

        fputs((char *)tokeniser->data, stderr); /* assume there is a null byte */
        fputc('\n', stderr);                    /* write new line */

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

int umn_parse_node_to_string(char *s, size_t max_len, struct UMN_PNode *pnode, int wrap /* wheter node should be wrapped within brackets*/)
{
    int n = 0;
    if (wrap && (max_len - n) > 1)
    {
        s[n++] = '(';
    }

    if (umn_kind_is(pnode->token.kind, UMN_KIND_BF_FUNCTION))
    {
        assert(pnode->children);

        n += umn_token_to_string(&pnode->token, s + n, max_len - n);

        if ((max_len - n) > 1)
        {
            s[n++] = '(';
        }

        for (size_t i = 0; i < pnode->child_count && (max_len - n) > 1; i++)
        {
            if (i > 0 && (max_len - n) > 2)
            {
                s[n++] = ',';
                s[n++] = ' ';
            }
            n += umn_parse_node_to_string(s + n, max_len - n, pnode->children + i, 0);
        }

        if ((max_len - n) > 1)
        {
            s[n++] = ')';
        }
    }
    else if (pnode->child_count > 0)
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
    else
    {
        n += umn_token_to_string(&pnode->token, s + n, max_len - n);
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

/*
    copy a parse node into an arena
    @param pnode allocated buffer to copy nodes into
*/
struct UMN_PNode *umn_parse_node_copy(struct UMN_Arena *arena, struct UMN_PNode *pnode)
{
    size_t node_capacity = 50;
    /* allocate buffer fornode to be copied into */
    struct UMN_PNode *dest_node = umn_arena_alloc(arena, (sizeof(struct UMN_PNode)) * node_capacity);
    assert(dest_node != NULL);

    size_t curr_idx = 0; /* current insert index */

    /* copy node into dest buffer */
    memcpy(dest_node, pnode, sizeof(struct UMN_PNode));
    curr_idx++;

    struct UMN_PNode *node = NULL; /* pointer to a node to copy the children from */

    /* stack of node indices */
    size_t node_idx = 0;
    struct UMN_Stack *idx_stack = umn_stack_init(arena, 40, sizeof(node_idx));
    assert(idx_stack != NULL);

    assert(umn_stack_push(idx_stack, &node_idx, arena) == 0);

    while (umn_stack_pop(idx_stack, &node_idx) == 0)
    {
        node = dest_node + node_idx;

        if (node->child_count == 0)
        {
            continue;
        }

        /* check if the children fit into the child nodes */
        if ((curr_idx + node->child_count) >= node_capacity)
        {
            /* increase the capacity */
            node_capacity = node_capacity * 2;
            /* reallocate the buffer */
            struct UMN_PNode *tmp = umn_arena_realloc(arena, dest_node, sizeof(struct UMN_PNode) * node_capacity), *tmp_node;
            assert(tmp != NULL);
            if (tmp != dest_node)
            { /* fix the pointers */
                for (size_t i = 0; i < node_idx; i++)
                {
                    tmp_node = tmp + i;
                    if (tmp_node->child_count == 0)
                    {
                        continue;
                    }
                    /* fix pointer */
                    size_t j = ((size_t)tmp_node->children - (size_t)dest_node) / (size_t)(sizeof(struct UMN_PNode));
                    tmp_node->children = tmp + j;
                }

                dest_node = tmp;
            }
        }

        node = dest_node + node_idx;
        memcpy(dest_node + curr_idx, node->children, (sizeof(struct UMN_PNode)) * node->child_count);
        node->children = dest_node + curr_idx;
        curr_idx += node->child_count;

        for (size_t i = 0; i < node->child_count; i++)
        {
            node_idx = curr_idx - 1 - i;
            if (umn_stack_push(idx_stack, &node_idx, arena))
            {
                assert(0); /* resizing stack not implemented */
            }
        }
    }

    umn_arena_free(arena, idx_stack);
    return dest_node;
}

/* approach from <https://github.com/Snorkungen/expression/blob/master/expression_tree_builder2.py> */
struct UMN_PNode *umn_parse(struct UMN_Arena *ret_arena, char *data)
{
    puts(data);
    putchar('\n');

    assert(ret_arena != NULL);
    struct UMN_PNode pnode = {0};

    struct UMN_Tokeniser tokeniser = {
        .data = data,
        .data_length = strlen(data),

        .position = 0,

        .keywords = UMN_keywords,
        .keywords_count = UMN_KWORD_COUNT,
    };

    /* initialse the arena */
    struct UMN_Arena *arena = umn_arena_init(sizeof(struct UMN_PNode) * 200);
    assert(arena != NULL);

    /* the stack was an idea but let's just use an array */
    /* instead of node list use a stack */
    struct UMN_Stack *node_stack = umn_stack_init(arena, 50 /* have the space for 30 elements */, sizeof(struct UMN_PNode));
    assert(arena != NULL);

    struct UMN_Stack *child_stack = umn_stack_init(arena, 10, sizeof(struct UMN_PNode));
    assert(child_stack != NULL);

    /* keep a record of the stack base and end for a bracket pair */
    struct UMN_Bracket_Location
    {
        size_t begin;
        size_t end;
        size_t depth;
        size_t hack; /* if the bracket location is set by further down parsing logic */
    };
    struct UMN_Stack *bracket_stack = umn_stack_init(arena, 10 /* maximum bracket depth */, sizeof(struct UMN_Bracket_Location));
    struct UMN_Bracket_Location bracket_location = {0};
    struct UMN_Stack *bracket_begin_stack = umn_stack_init(arena, bracket_stack->element_capacity, sizeof(size_t));

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
            pnode = pnode;
            goto error_bad;
        }
        else if (umn_kind_compare(pnode.token.kind, UMN_KIND_EOF))
        {
            break;
        }

        /* this does not handle nesting ...*/
        if (umn_kind_compare(pnode.token.kind, UMN_KIND_OBRACKET))
        {
            /* save the current stack index */
            bracket_location.begin = node_stack->index;
            umn_stack_push(bracket_begin_stack, &bracket_location.begin, NULL);
            continue;
        }
        else if (umn_kind_compare(pnode.token.kind, UMN_KIND_CBRACKET))
        {
            /* fetch the opening bracket and push the current stack index */
            assert(umn_stack_pop(bracket_begin_stack, &bracket_location.begin) == 0);

            bracket_location.depth = bracket_begin_stack->index + 1;

            bracket_location.end = node_stack->index;
            umn_stack_push(bracket_stack, &bracket_location, NULL);
            continue;
        }

        /* push node onto stack */
        assert(umn_stack_push(node_stack, &pnode, NULL) == 0);
    } while (umn_kind_compare(pnode.token.kind, UMN_KIND_EOF) == 0);

    /* include the entire expression */
    bracket_location.begin = 0;
    bracket_location.end = node_stack->index;
    bracket_location.depth = 0;
    umn_stack_push(bracket_stack, &bracket_location, NULL);

    /* stack of numbers to be used by function */
    struct UMN_Stack *index_stack = umn_stack_init(arena, node_stack->element_capacity, sizeof(size_t));
    assert(index_stack != NULL);

    struct UMN_PNode *curr = NULL, *prev = NULL, *next = NULL;

    size_t base = 0;
    size_t prev_depth = 0;

    /* assume the values are zero */
    int *delete_array = umn_arena_alloc(arena, sizeof(int) * (bracket_stack->element_capacity + 1));
    memset(delete_array, 0, sizeof(int) * (bracket_stack->element_capacity + 1));

    umn_stack_reverse(bracket_stack);
    while (umn_stack_pop(bracket_stack, &bracket_location) == 0)
    { /* do actual logic interpretting tokens and build a tree of the node-tokens*/

        /* get the delete sum ... */
        int delete_sum = 0;
        for (size_t i = 0; i <= bracket_location.depth; i++)
        {
            delete_sum += delete_array[i];
        }

        base = bracket_location.begin - delete_sum;
        node_stack->index = bracket_location.end - delete_sum;

        if (bracket_location.depth < prev_depth)
        {
            node_stack->index -= delete_array[prev_depth];
            delete_array[prev_depth] = 0;
        }

        /* func(a, b) => (func(a, b)) */
        /* done to prevent oddities such as func "(func a, b), c" being understood as "func(func(a, b, c))" */
        if (base > 0 && bracket_location.hack == 0)
        {
            struct UMN_PNode *prec_node = (struct UMN_PNode *)(node_stack->data + (base - 1) * node_stack->element_size);
            if (umn_kind_is(prec_node->token.kind, UMN_KIND_BF_FUNCTION) && prec_node->child_count == 0)
            {
                /* push a new bracket location that includes the function  */
                struct UMN_Bracket_Location temp_bl = {.begin = bracket_location.begin - 1, .end = bracket_location.end, .depth = bracket_location.depth++, .hack = 1};
                umn_stack_push(bracket_stack, &temp_bl, NULL);
            }
        }

        char oper_prec_level = 6; /* [brackets are handled seperately] unary_operators(3) functions(5), exponentiation(4), multiply/division(2) addition/subtraction(1) */
        while (--oper_prec_level > 0)
        {
            size_t i;
            for (i = base; i < node_stack->index; i++)
            {
                int curr_is_phantom = 0; /* indicate whether curr actually exists in the node stack */
                curr = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i));
                prev = (i > base) ? (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (i - 1))) : NULL;
                next = ((i + 1) < node_stack->index) ? (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * (i + 1)))
                                                     : NULL;

                /* handle functions "f(x, y)" */
                if (umn_kind_is(curr->token.kind, UMN_KIND_BF_FUNCTION) && curr->child_count == 0 && oper_prec_level == 5)
                {
                    /* What this think does not support functions given no arguments */
                    /* There might be a flaw since the logic does not know what brackets are */
                    /* read the next node if there is not a COMMA to seperate the there are problems */

                    if (next == NULL || (umn_kind_is(next->token.kind, UMN_KIND_BF_OPERATOR) && next->children == 0))
                    {
                        umn_parse_print_error(&tokeniser, curr->token);
                        fputs("umn_parse: value expected #861\n", stderr);
                        memcpy(&pnode, curr, sizeof(struct UMN_PNode));
                        goto error_bad;
                    }

                    /* this is dumb ... deals with stuff like "func d, func a, b, c" to be read as "func(d, func(a, b, c))" */
                    index_stack->index = 0;

                    int elements_to_shift_by = 0, value_expected = 1;
                    child_stack->index = 0;

                    for (size_t j = i + 1; j < node_stack->index; j++)
                    {
                        next = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * j * node_stack->direction));

                        if (value_expected == 1)
                        {
                            /* check that the value is valid */
                            if (umn_kind_is(next->token.kind, UMN_KIND_BF_OPERATOR) && next->children == 0)
                            {
                                break; /* i think this would actually should be an error */
                            }

                            umn_stack_push(child_stack, next, arena);

                            if (umn_kind_is(next->token.kind, UMN_KIND_BF_FUNCTION) && next->child_count == 0)
                            {
                                assert(umn_stack_push(index_stack, &child_stack->index, NULL) == 0);
                            }
                            else
                            {
                                value_expected = 0;
                            }
                        }
                        else if (value_expected == 0)
                        {
                            if (!umn_kind_compare(next->token.kind, UMN_KIND_COMMA))
                            {
                                break; /* comma is the only value expected */
                            }

                            value_expected = 1;
                        }

                        elements_to_shift_by++;
                    }

                    size_t nfs_val = 0;
                    while (umn_stack_pop(index_stack, &nfs_val) == 0 && nfs_val > 0)
                    {
                        /* validate that the nested function has arguments ... */
                        if (nfs_val == child_stack->index)
                        {
                            umn_parse_print_error(&tokeniser, ((struct UMN_PNode *)(child_stack->data + child_stack->element_size * (nfs_val - 1)))->token);
                            fputs("umn_parse: value expected #934\n", stderr);
                            memcpy(&pnode, ((struct UMN_PNode *)(child_stack->data + child_stack->element_size * (nfs_val - 1))), sizeof(struct UMN_PNode));
                            goto error_bad;
                        }

                        /* assigne next to the nested function that is being processed */
                        next = (struct UMN_PNode *)(child_stack->data + child_stack->element_size * (nfs_val - 1));

                        if (next->child_count)
                        {
                            continue; /* function already has children ignore */
                        }

                        next->child_count = child_stack->index - nfs_val;
                        next->children = umn_arena_alloc(arena, (sizeof(struct UMN_PNode) * next->child_count));

                        memcpy(next->children,
                               child_stack->data + (child_stack->element_size * nfs_val),
                               child_stack->element_size * next->child_count);

                        child_stack->index -= next->child_count;
                    }

                    /* the shifting of values should actually be a function */
                    {
                        curr->child_count = child_stack->index;
                        curr->children = umn_arena_alloc(arena, (sizeof(struct UMN_PNode) * curr->child_count));

                        memcpy(curr->children, child_stack->data, sizeof(struct UMN_PNode) * curr->child_count);

                        memmove(
                            node_stack->data + ((i + 1) * node_stack->element_size),
                            node_stack->data + ((i + elements_to_shift_by + 1) * node_stack->element_size),
                            (node_stack->element_capacity * node_stack->element_size) - ((i + elements_to_shift_by + 1) * node_stack->element_size));

                        node_stack->index -= elements_to_shift_by;
                    }

                    continue;
                }

                /* intercept and check if the following is a case for implicit multiplicatio */
                if (UMN_KIND_OPERATOR_PREC_LEVEl(UMN_KIND_MULT) == oper_prec_level)
                {
                    /* check that the following tokens imply multiplication */
                    if (umn_kind_is(curr->token.kind, UMN_KIND_BF_KEYWORD) && curr->child_count == 0)
                    {
                        continue;
                    }
                    else if (next == NULL || (umn_kind_is(next->token.kind, UMN_KIND_BF_KEYWORD) && next->child_count == 0))
                    {
                        continue;
                    }

                    prev = curr; /* set previous to current */
                    /* it can be assumed that the following state implies that the current and next node should be multiplied */
                    struct UMN_PNode mult_node = {
                        .token = {.kind = UMN_KIND_MULT, .value = {(size_t)'*' | ((size_t)'*' << (56)), 0}, .begin = curr->token.end - 1, .end = next->token.begin},
                        .child_count = 0,
                    };
                    curr = &mult_node;
                    curr_is_phantom = 1;
                }

                /* for testing only care about operators */
                if (!umn_kind_is_operator(curr->token.kind) || curr->child_count > 0)
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
                    memcpy(&pnode, curr, sizeof(struct UMN_PNode));
                    goto error_bad;
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
                    if (prev == NULL || (umn_kind_is(prev->token.kind, UMN_KIND_BF_KEYWORD) && prev->child_count == 0))
                    {
                        int offset = 1;
                        int minus_count = umn_kind_compare(curr->token.kind, UMN_KIND_SUB) ? 1 : 0;

                        /* Inspired by function nesting nest unary operators */
                        child_stack->index = 0;
                        index_stack->index = 0;
                        int elements_to_shift_by = 0;

                        assert(umn_stack_push(index_stack, &child_stack->index, NULL) == 0); /* push the current unary operator onto the index stack so that the following loop can do the logic part*/
                        assert(umn_stack_push(child_stack, curr, NULL) == 0);

                        for (size_t j = i + 1; j < node_stack->index; j++)
                        {
                            next = (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * j));

                            if (!umn_kind_is(next->token.kind, UMN_KIND_BF_UNARY) && umn_kind_is(next->token.kind, UMN_KIND_BF_KEYWORD) && next->child_count == 0)
                            {
                                break;
                            }

                            if (umn_kind_is(next->token.kind, UMN_KIND_BF_UNARY) && next->child_count == 0)
                            {
                                elements_to_shift_by++;
                                assert(umn_stack_push(index_stack, &child_stack->index, NULL) == 0);
                                assert(umn_stack_push(child_stack, next, NULL) == 0);
                                continue;
                            }
                            else
                            {
                                elements_to_shift_by++;
                                assert(umn_stack_push(child_stack, next, NULL) == 0);
                                break;
                            }
                        }

                        size_t un_index = 0;
                        while (umn_stack_pop(index_stack, &un_index) == 0)
                        {
                            if (un_index >= child_stack->index)
                            {
                                umn_parse_print_error(&tokeniser, (struct UMN_Token){.kind = curr->token.kind, .begin = curr->token.begin, .end = ((struct UMN_PNode *)(child_stack->data + child_stack->element_size * (child_stack->index - 1)))->token.end});
                                fputs("umn_parse: unary check next token is invalid #1089\n", stderr);
                                pnode.child_count = 0;
                                pnode.token.kind = curr->token.kind;
                                pnode.token.begin = curr->token.begin;
                                pnode.token.end = ((struct UMN_PNode *)(child_stack->data + child_stack->element_size * (child_stack->index - 1)))->token.end;
                                goto error_bad;
                            }

                            next = (struct UMN_PNode *)(child_stack->data + child_stack->element_size * (un_index)); /* this is the unary operator */
                            next->child_count = 1;
                            next->children = umn_arena_alloc(arena, sizeof(struct UMN_PNode));
                            memcpy(next->children, child_stack->data + (child_stack->element_size * (un_index + 1)), sizeof(struct UMN_PNode)); /* this is the unary operator's value */

                            if (umn_kind_is_keyword(next->children->token.kind) && next->children->child_count == 0) /* check that next is a value */
                            {
                                umn_parse_print_error(&tokeniser, next->token);
                                fputs("umn_parse: unary something went wrong #1098\n", stderr);
                                memcpy(&pnode, next, sizeof(struct UMN_PNode));
                                goto error_bad;
                            }

                            child_stack->index -= 1;
                        }

                        if (child_stack->index != 1)
                        {
                            umn_parse_print_error(&tokeniser, curr->token);
                            fputs("umn_parse: unary next token not found ...#1116\n", stderr);
                            memcpy(&pnode, curr, sizeof(struct UMN_PNode));
                            goto error_bad;
                        }

                        {                                                              /* remove elements from the node stack*/
                            memcpy(curr, child_stack->data, sizeof(struct UMN_PNode)); /* copy updated curr from child stack */

                            /* remove n elements from node stack */
                            memmove(
                                node_stack->data + ((i + 1) * node_stack->element_size),
                                node_stack->data + ((i + elements_to_shift_by + 1) * node_stack->element_size),
                                (node_stack->element_capacity * node_stack->element_size) - ((i + elements_to_shift_by + 1) * node_stack->element_size));

                            node_stack->index -= elements_to_shift_by;
                        }

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

                    memcpy(&pnode, curr, sizeof(struct UMN_PNode));
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
                    if (curr_is_phantom)
                    {
                        elements_to_shift_by -= 1;
                    }

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
                    node_stack->index -= (elements_to_shift_by);

                    /* this is a hack to fix, a problem caused by setting curr to point to a node not in the node stack */
                    if (curr_is_phantom)
                    {
                        memcpy(node_stack->data + (i)*node_stack->element_size, &pnode, (sizeof(struct UMN_PNode)));
                    }
                    else
                    {
                        memcpy(node_stack->data + (i - 1) * node_stack->element_size, &pnode, (sizeof(struct UMN_PNode)));
                    }
                }

                // umn_parse_node_print(&pnode);
                // printf("stack index = %zu\n", node_stack->index);

                i -= 1; /* move index back to rectify that the stack has been m modified */
            }
        };

        delete_array[bracket_location.depth] += (bracket_location.end - bracket_location.begin) - (node_stack->index - base);
        prev_depth = bracket_location.depth;
    }

    putchar('\n');
    printf("nodes left = %zu\n", node_stack->index);
    for (size_t i = 0; i < node_stack->index; i++)
    {
        umn_parse_node_print(
            (struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)));
        umn_token_print(
            ((struct UMN_PNode *)(node_stack->data + (node_stack->element_size * i)))->token);
    }

    if (node_stack->index != 1)
    {
        fputs("umn_parse: something went wrong no root node found\n", stderr);
        goto error_bad;
    }

    struct UMN_PNode *ret_node = umn_parse_node_copy(ret_arena, node_stack->data);
    /* finally delete the arena */
    umn_arena_delete(arena);

    return ret_node;

error_bad:
    umn_arena_delete(arena);
    struct UMN_PNode *errnode = umn_arena_alloc(ret_arena, sizeof(struct UMN_PNode));
    memcpy(errnode, &pnode, sizeof(struct UMN_PNode));

    return errnode;
}

#endif /* UMN_H */