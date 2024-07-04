#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
    TODO: read command line arguments.
    [X] Arena allocator
    [ ] read arguments
        ap_read problem what is this -x -10, are thees to flags or is -10 a value ???
*/

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
    char alias;
    char *name;
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
    AP_FlagValue **values = marena_alloc(arenaptr, sizeof(AP_FlagValue) * (fcount + 1));

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
        fidx = 0;
        for (; fidx < fcount; fidx++)
        {
            if (
                (strlen(fname) == 1 && fname[0] == options[fidx].alias) || strcmp(fname, options[fidx].name))
            {
                break; /* a location found exiting loop */
            }
        }

        if (fidx >= fcount)
        {
            fidx = fcount;

            /* should there be some special handling that would treat the following as a value */
        }

        /* create a memory allocation for the value, that is coming */

        AP_FlagValue *fvalue = marena_alloc(arenaptr, sizeof(AP_FlagValue));

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
                fvalue = marena_alloc(arenaptr, sizeof(AP_FlagValue));
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
    char *value = marena_alloc(arenaptr, length);
    size_t offset = 0;

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

MN_Token *mn_commit(MArena *arenaptr, size_t token_type, char *buffer)
{
    size_t length = strlen(buffer);

    /* TODO do some sanity checks that the information given are valid*/

    MN_Token *t = marena_alloc(arenaptr, sizeof(MN_Token));

    if (token_type & MNTT_IDENT)
    {
        t->value.s = marena_alloc(arenaptr, length + 1);
        memcpy(t->value.s, buffer, length);
    }
    else if (token_type & MNTT_INT)
    {
        if ('0' == buffer[0])
        {
            perror("not implemented: converting string to integer 0xaa etc..");
            /* value is started by zero be smarter about parsing whatever the value could be */
        }
        else
        {
            t->value.i = atoi(buffer);
        }
    }
    else if (token_type & MNTT_FLOAT)
    {
        t->value.f = atof(buffer);
    }

    t->token_type = token_type;

    return t;
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
            MN_Token *t = mn_commit(arenaptr, token_type, buffer);
            if (head == NULL)
            {
                head = t;
                token = head;
            }
            else
            {
                t->prev = token;
                token->next = t;
                token = t;
            }

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

            if (isdigit(c))
            {
                buffer[bindex++] = c;
            }
            else if ('.' == c)
            {
                buffer[bindex++] = c;
                token_type = (token_type | MNTT_FLOAT) ^ MNTT_INT; /* set the token type to a numeric float */
            }
            else if (token_type & MNTT_INT && '0' == buffer[0] && isdigit(input[i /* index was incremented already */]) && ('b' == tolower(c) || 'x' == tolower(c)))
            /* leave room for checking 0x 0X 04324 0b 0B */
            {
                buffer[bindex++] = c;
            }
            else
            {
                MN_Token *t = mn_commit(arenaptr, token_type, buffer);
                if (head == NULL)
                {
                    head = t;
                    token = head;
                }
                else
                {
                    t->prev = token;
                    token->next = t;
                    token = t;
                }

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
        MN_Token *t = mn_commit(arenaptr, token_type, buffer);
        if (head == NULL)
        {
            head = t;
            token = head;
        }
        else
        {
            t->prev = token;
            token->next = t;
            token = t;
        }
    }

    return head;
}

int main(int argc, char **argv)
{
    void *farena = marena_init();

    AP_FlagName options[] = {
        {'x', "hex"},
        {'d', "decimal"},
        {'o', "octal"},
    };

    /* ap read could maybe pass in function is flag, for dealing with "-190" */
    AP_FlagValue **fvalues = ap_read(farena, argc, argv, 0, NULL);

    if (fvalues[0] == NULL)
    {
        perror("-x must be given");
        return 2;
    }

    char *value = ap_concat_value(farena, fvalues[0]);
    if (value == NULL)
    {
        perror("-x must be foollowed by a number");
        return 2;
    }
    printf("%s\n", value);

    MN_Token *t = mn_parse(farena, value);

    if (t->token_type & MNTT_INT == 0)
    {
        perror("something went wrong");
        return 2;
    }

    printf("\"%s\" = %#llx\n", value, t->value.i);

    /* do the processing of the flags */

    marena_free(farena);

    return 0;
}

const size_t MARENA_ALLOCATION_SIZE = (1024 * 8); /* ~8kB largest possible allocation is that much*/

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
    size_t remaining_bytes = (arenaptr->end - arenaptr->curr);
    if (remaining_bytes <= size_in_bytes)
    {
        /* assign a new arena chunk */

        if ((size_t)(arenaptr->end - (void *)arenaptr->begin) < size_in_bytes)
        {
            return NULL;
        }

        MArena *new_arenaptr = marena_init(); /* TODO: allow specifying the exact amount required, if the default allocation is smaller than needed */

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
