#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "umn.h"

typedef struct
{
    size_t length;
    size_t size;
    void *data;
} Array;

typedef struct MArena
{
    struct Marena *begin;
    void *curr, *end;    /* pointers to location where the data is stored */
    struct MArena *next; /* the stack is a linked-list of stacks */
} MArena;
MArena *marena_init(void);
void marena_free(MArena *arenaptr);
void *marena_alloc(MArena *arenaptr, size_t size_in_bytes);

/* AP_ prefix are for Argument Parser*/

typedef struct
{
    char *name;
    char alias;
} AP_FlagName;

typedef struct AP_FlagValue
{
    struct AP_FlagValue *tail; /* the tail is only accurate for the first element after that the tail is itself*/
    struct AP_FlagValue *next; /* so that the elements can be read from last LIFO */
    char *value;
} AP_FlagValue;

char ap_is_flag(char *arg)
{
    if (arg[0] != '-' || strlen(arg) < 2 || strcmp(arg, "--") == 0)
        return 0; /* not a flag */

    /* flags cannot begin with numerical values [0 - 9]*/
    char firstc = arg[1] == '-' ? *(arg + 2) : *(arg + 1);
    if (isdigit(firstc))
        return 0; /* not a flag, --0x3 -032 */

    return 1;
}
/* returns a list of pointers to flag values that is the same the lenght as the option count */
AP_FlagValue **ap_read(MArena *arenaptr, size_t argc, char **argv, size_t fcount, AP_FlagName options[])
{
    /* one extra option at the end is for undefined values */
    char *arg;
    char *fname;
    size_t fidx;
    AP_FlagValue **values = marena_alloc(arenaptr, sizeof(AP_FlagValue) * (fcount + 1)); /* leave room for one extra value where unknown flags are dumped */

    if (values == NULL)
    {
        return NULL;
    }

    for (size_t i = 1; i < argc; i++)
    {
        arg = argv[i];

        if (ap_is_flag(arg) == 0)
        {
            /* the current argument does not satisfy constraints to be an argumetn, -x --xx */
            continue;
        }

        /* get the name, move pointer forward, and in the previous if statement guarantee that the length is more than at least two */
        fname = arg[1] == '-' ? (arg + 2) : (arg + 1);

        /* determine the index for the flag*/
        for (fidx = 0; fidx < fcount; fidx++)
        {
            if ((strlen(fname) == 1 && fname[0] == options[fidx].alias) || strcmp(fname, options[fidx].name) == 0)
            {
                break; /* a location found leaving loop */
            }
        }

        if (fidx >= fcount)
        {
            fidx = fcount;

            /* should there be some special handling that would treat the following as a value */
        }

        /* create a memory allocation for the value, that is coming */

        AP_FlagValue *fvalue;
        if ((fvalue = marena_alloc(arenaptr, sizeof(AP_FlagValue))) == NULL)
        {
            return NULL;
        }

        fvalue->tail = fvalue;
        values[fidx] = fvalue;

        /* modify the linked list and append elements onto the list no clue if the following logic works*/
        if (values[fidx] == NULL || 1 /* for now only allow on appearance of a flag */)
        {
            fvalue->tail = fvalue;
            values[fidx] = fvalue;
        }
        else
        {
            values[fidx]->tail->next = fvalue;
            values[fidx]->tail = fvalue;
        }

        /* walk forward and consume the values following the flag */
        for (size_t j = i + 1; j < argc; j++)
        {
            char *val = argv[j];

            /* exit inner loop and set the pointer, if the value is a flag */
            if (ap_is_flag(val))
            {
                i = j - 1;
                break;
            }

            if (values[fidx]->tail->value != NULL)
            {
                if ((fvalue = (AP_FlagValue *)marena_alloc(arenaptr, sizeof(AP_FlagValue))) == NULL)
                {
                    return values;
                }

                values[fidx]->tail->next = fvalue;
                values[fidx]->tail = fvalue;
            }

            /* the question is, should the values get copied into a new buffer? */
            values[fidx]->tail->value = val; /* maybe copy string IDK */
        }
    }

    return values;
}

/* this is an implementation of a function that writes the disperate values into one buffer*/
char *ap_concat_value(MArena *arenaptr, AP_FlagValue *fvalueptr)
{
    /* 1st calculate the final size of the resultant string */

    size_t length = 0;
    size_t i = 0;
    AP_FlagValue *fvalue = fvalueptr;

    while (fvalue != NULL)
    {
        if (fvalue->value == NULL)
        {
            fvalue = fvalue->next;
            continue;
        }

        length += 1; /* always append a character, for each value , space for null terminator is convieniently accounted for*/
        for (i = 0; fvalue->value[i] != '\0'; i++)
        {
            length += 1;
        }

        fvalue = fvalue->next;
    }

    if (length == 0)
    {
        return NULL; /* maybe check fail instead idk */
    }

    /* 2nd copy the values onto the value buffer */
    char *value;
    size_t offset = 0;

    if ((value = marena_alloc(arenaptr, length)) == NULL)
    {
        return NULL;
    }

    fvalue = fvalueptr;

    while (fvalue != NULL)
    {
        if (fvalue->value == NULL)
        {
            fvalue = fvalue->next;
            continue;
        }

        length = strlen(fvalue->value);
        memcpy((value + offset), fvalue->value, length);

        offset += length;      /* increment offset */
        value[offset++] = ' '; /* add space and increment the offset*/

        fvalue = fvalue->next;
    }

    value[offset - 1] = '\0'; /* set last byte to null */

    return value;
}

/* MN prefix for mn stuff*/

const size_t MNTT_INFO_MASK = 0b1111; /* first (4) bits are reserved for encoding of information */
const size_t MNTT_NUMERIC = 1 << 4;   /* if set the value is either a float or and integer, (default integer) */
const size_t MNTT_IDENT = 1 << 5;     /* if set the value is a string value identity */
const size_t MNTT_TOKENS = 1 << 6;    /* if set the value is an other list of tokens */
const size_t MNTT_INT = 1 << 7;       /* if set the value is an integer */
const size_t MNTT_FLOAT = 1 << 8;     /* if set the value is a float*/

/* doubly linked list, because why not */
typedef struct MN_Token
{
    struct MN_Token *next, *prev;

    size_t token_type;
    union
    {
        signed long long int i;
        long double f;
        char *s;
    } value;
} MN_Token;

/* convert 0b11 => 3  */
long long mn_bintoll(char *s)
{
    long long result = 0;
    /* the strategy is to go to the end and walk backwards */

    size_t i = 0, length = strlen(s);

    if (length > 2 && '0' == s[0] && ('b' == s[1] || 'B' == s[1]))
    {
        length -= 2;
        s += 2;
    }

    for (i = 0; (length - i) > 0; i++)
    {
        if ('1' == s[length - i - 1])
        {
            result |= (1 << i);
        }
    }

    return result;
}

long long mn_hextoll(char *s)
{
    long long result = 0;

    size_t i = 0, length = strlen(s);

    if (length > 2 && '0' == s[0] && ('x' == s[1] || 'X' == s[1]))
    {
        length -= 2;
        s += 2;
    }

    for (i = 0; (length - i) > 0; i++)
    {
        long long c = s[length - i - 1];

        if (isdigit(c))
        {
            c -= 48;
        }
        else if (c >= 'A' && c <= 'F')
        {
            c = c - ('A') + 10;
        }
        else if (c >= 'a' && c <= 'f')
        {
            c = c - ('a') + 10;
        }
        else
        {
            c = 0;
        }

        result |= (c << (4 * i));
    }

    return result;
}

/* convert 0o17 => 15 */
long long mn_octtoll(char *s)
{
    long long result = 0;

    size_t i = 0, length = strlen(s);
    char c;

    if (length > 2 && '0' == s[0] && ('o' == s[1] || 'O' == s[1]))
    {
        length -= 2;
        s += 2;
    }

    for (i = 0; (length - i) > 0; i++)
    {
        c = s[length - i - 1];
        if (isdigit(c) && c < '8')
        {
            result |= ((c - '0') << (3 * i));
        }
    }

    return result;
}

MN_Token *mn_create_token(MArena *arenaptr, size_t token_type, char *buffer)
{
    size_t length = strlen(buffer);

    /* TODO do some sanity checks that the information given are valid*/

    MN_Token *t;
    if ((t = marena_alloc(arenaptr, sizeof(MN_Token))) == NULL)
    {
        return NULL;
    }

    if (token_type & MNTT_IDENT)
    {
        if ((t->value.s = marena_alloc(arenaptr, length + 1)) == NULL)
        {
            return NULL;
        };
        memcpy(t->value.s, buffer, length);
    }
    else if (token_type & MNTT_INT)
    {
        int modifier;
        if ('-' == buffer[0] & length > 1)
        {
            buffer += 1;
            modifier = -1;
        }
        else if ('+' == buffer[0] & length > 1)
        {
            buffer += 1;
            modifier = 1;
        }
        else
        {
            modifier = 1;
        }

        if ('0' == buffer[0] & length > 2)
        {
            /* value is started by zero be smarter about parsing whatever the value could be */

            switch (buffer[1])
            {
            case 'B':
            case 'b':
                t->value.i = mn_bintoll(buffer);
                break;
            case 'O':
            case 'o':
                t->value.i = mn_octtoll(buffer);
                break;
            case 'X':
            case 'x':
                t->value.i = mn_hextoll(buffer);
                break;
            default:
                t->value.i = 0;
            }
        }
        else
        {
            t->value.i = atoll(buffer);
        }

        /* make the number negative if that has been specified*/
        t->value.i *= modifier;
    }
    else if (token_type & MNTT_FLOAT)
    {
        t->value.f = atof(buffer);
    }

    t->token_type = token_type;

    return t;
}
/* create a token and add to the linked-list of tokens */
void mn_commit(MArena *arenaptr, MN_Token **head, MN_Token **token, size_t token_type, char *buffer)
{
    MN_Token *t = mn_create_token(arenaptr, token_type, buffer);

    if (t == NULL)
    {
        return; /* do nothing if failed to allocate memory*/
    }

    if (*head == NULL)
    {
        *head = t;
        *token = *head;
    }
    else
    {
        t->prev = *token;
        (*token)->next = t;
        *token = t;
    }
}

/* logic for checking that the current character is valid for the text representaition of a number */
/* validate that the character works for the give prefix i.e 0xaaa 0b1111 0o010117 */
int mn_char_is_valid_for_prefix(char prefix, char c)
{
    switch (prefix)
    {
    case 'B':
    case 'b':
        return '0' == c || '1' == c;
    case 'O':
    case 'o':
        return isdigit(c) && c < '8';
    case 'X':
    case 'x':
        return isxdigit(c);
    }
    return 0;
}

MN_Token *mn_parse(MArena *arenaptr, char *input)
{

    /* or should this be an array where then the values are, lets se how cumbersome it becomes to traverse the linke-list */
    MN_Token *head = NULL, *token = head;

    size_t i = 0;

    size_t token_type = 0;
    size_t bindex = 0; /* buffer index */
    char buffer[100] = {0};

    while (input[i] != '\0') /* be careful when moving the index to not exceed the input */
    {
        char c = input[i];
        i++;

        if (isspace(c))
        {
            if (token_type == 0)
                continue;

            if ('\0' != buffer[0] || 0 == bindex)
            {
                /* the token type is set but the buffer is not */
            }

            /* commit the current token type and buffer into the stuff */
            // TODO: create token and commit
            mn_commit(arenaptr, &head, &token, token_type, buffer);

            /* reset state */
            token_type = 0;
            memset(buffer, 0, 100); /* the buffer size is 100 */
            bindex = 0;
        }

        /* initialize token_type */
        if (token_type == 0)
        {
            if (isdigit(c))
                token_type = MNTT_NUMERIC | MNTT_INT; /* the value is assumed to be an integer */
            else
                token_type = MNTT_IDENT;
        }

        // TODO: support brackets

        if (token_type & MNTT_NUMERIC)
        {
            /* sanity check */
            if (bindex + 2 >= 100)
            {
                perror("mn_parse buffer exceeded");
                exit(1);
            }

            /* Validate that the current character works for the numeric type */
            if (isdigit(c) || (token_type & MNTT_INT && bindex > 1 && '0' == buffer[0] && mn_char_is_valid_for_prefix(buffer[1], c)))
            {
                buffer[bindex++] = c;
            }
            else if (token_type & MNTT_INT && '.' == c) /* is a token that informs that the following number is going to be a float */
            {
                buffer[bindex++] = c;
                token_type = (token_type | MNTT_FLOAT) ^ MNTT_INT; /* set the token type to a numeric float */
            }
            else if (token_type & MNTT_INT && '0' == buffer[0] && '\0' == buffer[1] && mn_char_is_valid_for_prefix(c, input[i /* index was incremented already */]))
            { /* check that the current character informs the logic that the following is a non decimal representation of the number */
                buffer[bindex++] = c;
            }
            else
            {
                mn_commit(arenaptr, &head, &token, token_type, buffer);

                /* reset state */
                token_type = MNTT_IDENT;
                memset(buffer, 0, 100); /* the buffer size is 100 */
                bindex = 0;
            }
        }

        if (token_type & MNTT_IDENT)
        {
            /* sanity check */
            if (bindex + 2 >= 100)
            {
                perror("mn_parse buffer exceeded");
                exit(1);
            }

            /* check special case if the following value is a digit and set, it the current character is - or +*/

            buffer[bindex++] = c;
        }
    }

    if (token_type != 0)
    {
        mn_commit(arenaptr, &head, &token, token_type, buffer);
    }

    return head;
}

/* convert number to string 7 => 0b111 */
char *lltobstr(MArena *arenaptr, long long n)
{
    char *s;
    if ((s = marena_alloc(arenaptr, 3 + sizeof(n) * 8 + 1)) == NULL)
    {
        return NULL;
    }

    size_t offset = 0, i = 0;

    /* treat values as unsigned where, so that -1 => -0b1 */
    if (n < 0)
    {
        s[i++] = '-';
        n *= -1; /* make num positive if the number was negative*/
    }

    /* write "0b" to the string*/
    s[i++] = '0';
    s[i++] = 'b'; /* could have switch that toggles 'B' */

    /* find the first byte with values */

    /* move offset until finds first byte with data */
    for (offset = 0; (n & ((0xffULL << 7 * 8)) >> ((offset) * 8)) == 0; offset++)
    {
        ; /* noop the above statement does some cursed B.S. */
    }

    offset = offset * 8; /* make a bit offset now */

    /* move offset untill it finds its first byte with a 1*/
    while (offset <= sizeof(n) * 8)
    {
        if (n & (1ULL << (sizeof(n) * 8 - (++offset))))
        {
            break;
        }
    }

    while (offset <= sizeof(n) * 8)
    {
        s[i++] = n & (1ULL << (sizeof(n) * 8 - offset++)) ? '1' : '0';
    }

    return s;
}

int main(int argc, char **argv)
{
    /* "0xa - (-0b11) = 13a_0o71öl_pastej∑" */
    // umn_parse("2 + 1 *4 + a");

    struct UMN_Arena *arenaptr = umn_arena_init(sizeof(struct UMN_PNode) * 1000);

   // umn_parse(arenaptr, "(1 *-1) - (10 * 2)"); /* this is wrong some how*/
    umn_parse(arenaptr, "1 * -+-a * 1 * 2 - 1");
    // umn_parse("--a / 1 * - 1");
    // umn_parse("1 * (a - b) + 1 + -a**2");
    // umn_parse("(1 + (a- 1) * (b - 1)) * (a)");
    // umn_parse("1 + (a * (b - 1) - (c - 1))");
    // umn_parse("(a + b) * (a ** (b - 1)) - -(a - b)");

    // umn_parse("a + testf(2 + 2)");
    // umn_parse("a + testf(2, b * (2 - a)) + 2");
    // umn_parse("testf testf( a -b, c), d + 2");       /* expected: testf(testf(a - b, c), d) */
    // umn_parse("testf testf( a -b, testf c), d + 2"); /* expected: testf(testf(a - b, testf(c)), d) */
    // umn_parse("testf testf(a), b");

    // umn_parse("testf(testf c)(a * b)");
    // umn_parse("testf a, testf c d");

    // umn_parse("2a + a"); /* TODO: add support for implicit multiplication, i.e. (2 * a) + a */

    // umn_parse("testf a, b 10");
    return 0; /* Ignore the below program seg faults fvalues points to a bad pointer */
    void *arena = marena_init();

    const size_t F_DECIMAL = 0, F_BINARY = 1, F_OCTAL = 2, F_HEX = 3, F_INTERACTIVE = 4, F_UNDEFINED = 5;

    AP_FlagName options[] = {
        {"decimal", 'd'},
        {"binary", 'b'},
        {"octal", 'o'},
        {"hex", 'x'},
        {"interactive", 'i'}, /* TODO: add support for interactively entering numbers*/
    };

    /* ap read could maybe pass in function is flag, for dealing with "-190" */
    AP_FlagValue **fvalues = ap_read(arena, argc, argv, (5 /* options is a constant */), options);
    AP_FlagValue *fvalue;

    if (fvalues == NULL)
    {
        perror("error: not enough memory");
        exit(1);
    }

    char *s;
    MN_Token *t;

    /*
        TODO: add support for configuring the output
    */

    /* if no flags give but argument give assume the value is a decimal argument */
    {
        size_t sum = 0, i = 0;
        for (i = 0; i < F_UNDEFINED; i++)
            sum += (size_t)fvalues[i];

        if (argc > 1 && sum == 0)
            fvalues[F_DECIMAL] = &(AP_FlagValue){0, 0, argv[1]};
    }

    if (fvalues[F_DECIMAL] != NULL)
    {
        /* NOTE: could seg fault if no value is passed*/
        if ((s = ap_concat_value(arena, fvalues[F_DECIMAL])) != NULL && (t = mn_parse(arena, s)) != NULL)
        {
            if ((t->token_type & MNTT_NUMERIC) == 0)
            {
                printf("\"%s\" = %s\n", s, t->value.s);
            }
            else if (t->token_type & MNTT_FLOAT)
            {
                printf("\"%s\" = %Lf\n", s, t->value.f); /* this program does not actually support floating point numbers*/
            }
            else
            {
                printf("\"%s\" = %lld\n", s, t->value.i);
            }
        }
    }

    if (fvalues[F_BINARY] != NULL)
    {
        if ((s = ap_concat_value(arena, fvalues[F_BINARY])) != NULL && (t = mn_parse(arena, s)) != NULL)
        {
            if (t->token_type & MNTT_NUMERIC && t->token_type & MNTT_INT)
            {
                char *binstr = lltobstr(arena, t->value.i);
                if (binstr == NULL)
                {
                    binstr = "0b0";
                }
                printf("\"%s\" = %s\n", s, binstr);
            }
        }
    }

    if (fvalues[F_OCTAL] != NULL)
    {
        if ((s = ap_concat_value(arena, fvalues[F_OCTAL])) != NULL && (t = mn_parse(arena, s)) != NULL)
        {
            if (t->token_type & MNTT_NUMERIC && t->token_type & MNTT_INT)
            {
                printf("\"%s\" = 0o%llo\n", s, t->value.i);
            }
        }
    }

    if (fvalues[F_HEX] != NULL)
    {
        if ((s = ap_concat_value(arena, fvalues[F_HEX])) != NULL && (t = mn_parse(arena, s)) != NULL)
        {
            if (t->token_type & MNTT_NUMERIC && t->token_type & MNTT_INT)
            {
                printf("\"%s\" = %#llx\n", s, t->value.i);
            }
        }
    }

    if (fvalues[F_UNDEFINED])
    {
        /* handle if given an unknown flag */
    }

    marena_free(arena);

    return 0;
}

const size_t MARENA_ALLOCATION_SIZE = (1024 * 4); /* ~4kB largest possible allocation is that much*/

/* initialize an arena of memory */
MArena *marena_init(void)
{
    MArena *arenaptr = malloc(MARENA_ALLOCATION_SIZE + sizeof(MArena));

    if (arenaptr == NULL)
    {
        return NULL;
    }

    /* first allocation is technically the arena itself*/
    arenaptr->begin = (void *)arenaptr;
    arenaptr->curr = (void *)((size_t)(arenaptr->begin) + sizeof(MArena)); /* because i do not trust that thiswilldo what i expect */
    arenaptr->end = arenaptr->curr + MARENA_ALLOCATION_SIZE;

    arenaptr->next = NULL;

    return arenaptr;
}

/* walk the linked list of ptrs */
MArena *marena_get_active(MArena *arenaptr)
{
    while (arenaptr->next != NULL)
    {
        arenaptr = arenaptr->next;
    }

    return arenaptr;
}

/* free a marena allocation */
void marena_free(MArena *arenaptr)
{
    MArena *nextptr = arenaptr;

    while (nextptr != NULL)
    {
        nextptr = arenaptr->next;
        free(arenaptr);
        arenaptr = nextptr;
    }
}

/* allocate an array of bytes, returns null if failed to allocate*/
void *marena_alloc(MArena *arenaptr, size_t size_in_bytes)
{
    void *allocation = NULL;

    arenaptr = marena_get_active(arenaptr);

    /* check that there is enough space to allocate the requested memory*/
    size_t remaining_bytes = ((char *)(arenaptr->end) - (char *)(arenaptr->curr));
    if (remaining_bytes <= size_in_bytes)
    {
        /* assign a new arena chunk */

        if ((size_t)(arenaptr->end - (size_t)arenaptr->begin) < size_in_bytes)
        {
            return NULL;
        }

        MArena *new_arenaptr = marena_init(); /* TODO: allow specifying the exact amount required, if the default allocation is smaller than needed */

        if (new_arenaptr == NULL)
        {
            return NULL;
        }

        arenaptr->next = new_arenaptr;
        arenaptr = arenaptr->next;
    }

    allocation = arenaptr->curr;

    /* align the size in bytes to 32 bits*/
    /* should probably align the the data to a boundary*/

    /* move the curr pointer forward x amount */
    arenaptr->curr += size_in_bytes;

    return allocation;
}
