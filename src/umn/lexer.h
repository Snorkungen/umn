#ifndef UMN_LEXER_H
#define UMN_LEXER_H

#define UMN_DEBUG_HERE(msg) printf("%d:%s %s\n", __LINE__, __func__, msg)

/* The aim of this rewrite is to use the paradime that i have started with utils etc..., */
/* Most important is to do the thing ... */
/* umn_token_enc(token) */
/* umn_token_data(token)*/
/* umn_token_flag(token)*/

#include "./utils.h"

#include <stdio.h>  /* printf, puts */
#include <ctype.h>  /* tolower, toupper, isdigit, isxdigit, isspace */
#include <string.h> /* strtod, strtol, strtoul, memset, strlen strncmp */

typedef enum
{
  umn_Enc_Integer_Dec = '\0',
  umn_Enc_Integer_Hexadec = 'x',
  umn_Enc_Integer_Binary = 'b',
  umn_Enc_Integer_Octal = '0',

  umn_Enc_Fraction_E = 'E',
  umn_Enc_Integer_E = umn_Enc_Fraction_E,

  umn_Enc_String_1 = '\'',
  umn_Enc_String_2 = '\"',
} umn_Token_Encoding;

typedef struct
{
  const char *s;
  const uint32_t flags;
  const int32_t data; /* this is an int */
} umn_Symbol;

#define UMN_SYMBOLS_MAX_COUNT 48
typedef struct
{
  uint32_t count;
  const umn_Symbol *items;
} umn_Lexer_Symbols;

typedef struct
{
  char const *data;

  size_t position, next_position; /* Location of where to read next */
  size_t line_begin;
  size_t separation_pos; /* location lexer logic uses for stuff */

  umn_Lexer_Symbols symbols; /* View ... a slice is resize able a view is fixed */
} umn_Lexer;

typedef uint64_t umn_Token_Kind;
typedef struct
{
  umn_Token_Kind kind;
  uint64_t begin;
  uint32_t length;
  /* how many bytes this is offset from the lines start */
  uint32_t line_offset;

  /* having union will save bits but is it worth it ...*/

  umn_Token_Encoding encoding;
  uint32_t flags; /* generic thing that can be used for stuff */
  int32_t data;
} umn_Token;

static const umn_Token_Kind UMN_KEOF = 0,
                            UMN_KERR = (~(((umn_Token_Kind)-1) >> 4)),
                            UMN_KERR_SEP = (~(((umn_Token_Kind)-1) >> 3));

static const umn_Token_Kind UMN_K__RESERVED__ = 0xFF0000,
                            UMN_K__1 = 0x10000,
                            UMN_K__2 = UMN_K__1 << 1,
                            UMN_K__3 = UMN_K__1 << 2,
                            UMN_K__4 = UMN_K__1 << 3,
                            UMN_K__5 = UMN_K__1 << 4,
                            UMN_K__6 = UMN_K__1 << 5,
                            UMN_K__7 = UMN_K__1 << 6,
                            UMN_K__8 = UMN_K__1 << 7;

static const umn_Token_Kind UMN_KSTRING = 0x400,
                            UMN_KLITERAL = 0x200,
                            UMN_KSYMBOL = UMN_KLITERAL | 0x80; /* TODO: remove reliance on the fact that ksymbol is also a literal */
static const umn_Token_Kind UMN_KNUMERIC = 0x100,
                            UMN_KINTEGER = UMN_KNUMERIC | 0x2,
                            UMN_KFRACTION = UMN_KNUMERIC | 0x1;

int umn_lexer_next(umn_Lexer *lexer, umn_Token *token);
int umn_lexer_peek(const umn_Lexer *lexer, umn_Token *token);
int umn_lexer_take(umn_Lexer *lexer, const umn_Token *token);
int umn_lexer_give(umn_Lexer *lexer, const umn_Token *token); /* reset the position to just before the token */

void umn_token_print(const umn_Lexer *lexer, const umn_Token *token);

int64_t umn_token_readi(const umn_Lexer *lexer, const umn_Token *token);
double umn_token_readf(const umn_Lexer *lexer, const umn_Token *token);
char *umn_token_strncpy(const umn_Lexer *lexer, const umn_Token *token, char *dest, size_t dsize);
int umn_sb_push_token(umn_sb_t *sb, const umn_Lexer *lexer, const umn_Token *token);

int umn_token_litncmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal, const size_t litsize);
static int umn_token_litcmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal);

/* compares two token if they equal */
static int umn_token_is(const umn_Lexer *lexer, const umn_Token *a, const umn_Token *b);
/* compares a token with a literal */
static int umn_token_issymbol(const umn_Lexer *lexer, const umn_Token *token, const char *literal);

void umn_token_print(const umn_Lexer *lexer, const umn_Token *token);
void umn_token_print_error(const umn_Lexer *lexer, const umn_Token *token);

#ifdef UMN_LEXER_IMPLEMENTATION

static int umn_lexer__match_symbol(umn_Lexer *lexer, umn_Token *token);

static wchar_t umn_lexer_decode_utf8(umn_Lexer *lexer);
static inline wchar_t umn_lexer_decode_utf8(umn_Lexer *lexer)
{
  if (lexer->data[lexer->position] & 0x80)
    UMN_TODO("Actually support decoding of utf8");

  lexer->next_position = lexer->position + 1;
  return lexer->data[lexer->position];
}

static bool umn_lexer_read_integer(umn_Lexer *lexer, umn_Token *token, wchar_t curr);
static inline bool umn_lexer_read_integer(umn_Lexer *lexer, umn_Token *token, wchar_t curr)
{
  if (token->encoding == umn_Enc_Integer_Dec && curr == '.')
  {
    token->kind = UMN_KFRACTION;
    lexer->separation_pos = lexer->position;
    return true;
  }

  /* NOTE: I do not know how the utf8 parsing will effect this */
  if (token->begin == (lexer->position - 1) && lexer->data[token->begin] == '0')
  {
    /* attempt to read the thing ... */
    if (umn_isodigit(curr))
    {
      token->encoding = umn_Enc_Integer_Octal;
      return true; /* Octal, does not have a separation char */
    }
    else if (tolower(curr) == umn_Enc_Integer_Binary)
      token->encoding = umn_Enc_Integer_Binary;
    else if (tolower(curr) == umn_Enc_Integer_Hexadec)
      token->encoding = umn_Enc_Integer_Hexadec;
    else
    {
      return false;
    }

    lexer->separation_pos = lexer->position;
    return true;
  }
  else if (lexer->data[token->begin] != '0' && token->encoding == umn_Enc_Integer_Dec && (curr) == umn_Enc_Integer_E)
  {
    token->encoding = umn_Enc_Integer_E;
    lexer->separation_pos = lexer->position;
    return true;
  }
  else if (token->encoding == umn_Enc_Integer_E && lexer->position == (lexer->separation_pos + 1) && (curr == '-' || curr == '+'))
  {
    lexer->separation_pos = lexer->position;
    return true;
  }

  switch (token->encoding)
  {
  case umn_Enc_Integer_Binary:
    return umn_isbdigit(curr);
  case umn_Enc_Integer_Octal:
    return umn_isodigit(curr);
  case umn_Enc_Integer_Hexadec:
    return umn_isxdigit(curr);
  case umn_Enc_Integer_Dec:
  default: /* do nothing */
    return umn_isdigit(curr);
  }
}

static bool umn_lexer_read_fraction(umn_Lexer *lexer, umn_Token *token, wchar_t curr);
static inline bool umn_lexer_read_fraction(umn_Lexer *lexer, umn_Token *token, wchar_t curr)
{
  if (toupper(curr) == umn_Enc_Fraction_E)
  {
    token->encoding = umn_Enc_Fraction_E;
    lexer->separation_pos = lexer->position;
    return true;
  }
  else if (token->encoding == umn_Enc_Integer_E && lexer->position == (lexer->separation_pos + 1) && (curr == '-' || curr == '+'))
  {
    lexer->separation_pos = lexer->position;
    return true;
  }

  return isdigit(curr);
}

int umn_lexer_next(umn_Lexer *lexer, umn_Token *token)
{
  wchar_t curr;
  *token = (umn_Token){0};

  lexer->separation_pos = (size_t)-1;

  /* before we begin skip whitespace */
  while (lexer->data[lexer->position] != '\0' && ((curr = lexer->data[lexer->position]) <= ' ' || isspace(curr)))
  {
    lexer->position++;
    if (curr == '\n')
      lexer->line_begin = lexer->position;
  }

  if (lexer->data == NULL || lexer->data[lexer->position] == '\0')
  {
    token->kind = UMN_KEOF;
    return 0;
  }

  /* INIT */
  /* Read first char */
  curr = umn_lexer_decode_utf8(lexer);

  assert(token->kind == 0);
  /* init token */
  token->begin = lexer->position;
  token->line_offset = lexer->position - lexer->line_begin;
  token->kind = isdigit(curr) ? UMN_KINTEGER : UMN_KLITERAL;

  if (curr == umn_Enc_String_1 || curr == umn_Enc_String_2)
  {
    token->kind = UMN_KSTRING;
  }

  bool b = false;
  while (token->kind == UMN_KLITERAL)
  {
    switch (curr)
    {
    case '\0':
    case '\\':
    case umn_Enc_String_1:
    case umn_Enc_String_2:
      b = true;
    }

    if (b || isspace(curr))
      break;

    lexer->position = lexer->next_position;
    curr = umn_lexer_decode_utf8(lexer);
  }

  /* read string, returns when done */
  if (token->kind == UMN_KSTRING)
  {
    token->encoding = (umn_Token_Encoding)curr;
    token->begin = lexer->position = lexer->next_position;

    unsigned count = 0;
    while (lexer->data[(++lexer->position)] != '\0' && (lexer->data[lexer->position] != curr || (count & 1)))
    {
      /* Wow branch-less so cool
      if lexer->data[lexer->position] == '\\'
      count += 1
      else
      count = 0
      */
      count = (unsigned)(lexer->data[lexer->position] == '\\') * (count + 1);
    }
    if (lexer->data[lexer->position] != curr)
      token->kind |= UMN_KERR;
    else
    {
      token->length = lexer->position - token->begin;
      lexer->position++;
      return 0;
    }
  }

  while (token->kind == UMN_KINTEGER && umn_lexer_read_integer(lexer, token, curr))
  {
    curr = lexer->data[++lexer->position];
  }

  while (token->kind == UMN_KFRACTION && umn_lexer_read_fraction(lexer, token, curr))
  {
    curr = lexer->data[++lexer->position];
  }

  /* NOTE: how does this account for utf-8, but the above logic only cares obout ascii*/
  if (lexer->separation_pos == (lexer->position - 1))
  {
    token->kind |= UMN_KERR_SEP;
    lexer->position++;
  }

  token->length = lexer->position - token->begin;

  if ((token->kind & UMN_KERR))
  {
    return -1;
  }

  assert(token->length > 0);

  if (UMN_KLITERAL == token->kind)
    return umn_lexer__match_symbol(lexer, token); /* tc */

  return 0;
}

inline int umn_lexer_peek(const umn_Lexer *lexer, umn_Token *token)
{
  umn_Lexer tmp = *lexer;
  return umn_lexer_next(&tmp, token);
}

inline int umn_lexer_take(umn_Lexer *lexer, const umn_Token *token)
{
  lexer->position = token->begin + token->length;
  lexer->line_begin = token->begin - token->line_offset;
  return 0;
}

inline int umn_lexer_give(umn_Lexer *lexer, const umn_Token *token)
{
  lexer->position = token->begin;
  lexer->line_begin = token->begin - token->line_offset;
  return 0;
}

static inline int umn_lexer__match_symbol(umn_Lexer *lexer, umn_Token *token)
{
  /* just statically allocated on the stack a thing */
  const umn_Symbol *symbol;
  const char *symbol_s;
  int best = -1, best_count = 0, count = 0;

  for (unsigned i = 0; i < lexer->symbols.count; i++)
  {
    symbol_s = lexer->symbols.items[i].s;

    if (symbol_s == NULL)
      continue;

    count = 0;
    while (lexer->data[token->begin + count] == symbol_s[count] && symbol_s[count] != '\0')
      count++;

    if (symbol_s[count] == '\0' && count > best_count)
    {
      best = i;
      best_count = count;
    }
  }

  if (best >= 0)
  {
    token->kind = UMN_KSYMBOL;
    token->length = best_count;
    lexer->position = token->begin + token->length;

    /* we still need a better way of doing this  */
    token->data = lexer->symbols.items[best].data;
    token->flags = lexer->symbols.items[best].flags;

    return 0;
  }

  static uint32_t cached_lens[UMN_SYMBOLS_MAX_COUNT];
  assert(ARRAY_LEN(cached_lens) > lexer->symbols.count);

  for (unsigned i = 0; i < lexer->symbols.count; i++)
    cached_lens[i] = strlen(lexer->symbols.items[i].s);

  for (unsigned i = 1; i < token->length; i++)
  {

    for (unsigned j = 0; j < lexer->symbols.count; j++)
    {
      symbol = lexer->symbols.items + j;

      if (((token->length - i) >= cached_lens[j]) /* symbol must fit into the remaining token */ &&
          strncmp(&lexer->data[token->begin + i], symbol->s, cached_lens[j]) == 0)
      {
        /* we's found something quit */
        token->length = i;
        lexer->position = token->begin + token->length;

        return 0;
      }
    }
  }

  return 0;
}

int64_t umn_token_readi(const umn_Lexer *lexer, const umn_Token *token)
{
  char const *s_beg = lexer->data + token->begin;
  char *s_end = (char *)s_beg + token->length;

  switch (token->encoding)
  {
  case umn_Enc_Integer_Binary:
    return strtol(s_beg + 2, &s_end, 2);
  case umn_Enc_Integer_Octal:
    return strtol(s_beg, &s_end, 8);
  case umn_Enc_Integer_Hexadec:
    return strtol(s_beg, &s_end, 16);
  default:
    return strtol(s_beg, &s_end, 10);
  }
}

uint64_t umn_token_readu(const umn_Lexer *lexer, const umn_Token *token)
{
  char const *s_beg = lexer->data + token->begin;
  char *s_end = (char *)s_beg + token->length;

  switch (token->encoding)
  {
  case umn_Enc_Integer_Binary:
    return strtoul(s_beg + 2, &s_end, 2);
  case umn_Enc_Integer_Octal:
    return strtoul(s_beg, &s_end, 8);
  case umn_Enc_Integer_Hexadec:
    return strtoul(s_beg, &s_end, 16);
  default:
    return strtoul(s_beg, &s_end, 10);
  }
}

double umn_token_readf(const umn_Lexer *lexer, const umn_Token *token)
{
  char const *s_beg = lexer->data + token->begin;
  char *s_end = (char *)s_beg + token->length;

  return strtod(s_beg, &s_end);
}

int umn_sb_push_token(umn_sb_t *sb, const umn_Lexer *lexer, const umn_Token *token)
{
  return umn_sb_pushsn(sb, lexer->data + token->begin, token->length);
}

char *umn_token_strncpy(const umn_Lexer *lexer, const umn_Token *token, char *dest, size_t dsize)
{
  if (token->length < dsize)
  {
    dsize = token->length;
    dest[token->length] = '\0';
  }
  else
    return NULL;

  return strncpy(dest, lexer->data + token->begin, dsize);
}

int umn_token_litncmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal, const size_t litsize)
{
  if (litsize == token->length)
    return strncmp(lexer->data + token->begin, literal, token->length);
  return -1;
}

inline int umn_token_litcmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal)
{
  return umn_token_litncmp(lexer, token, literal, strlen(literal));
}

inline int umn_token_is(const umn_Lexer *lexer, const umn_Token *a, const umn_Token *b)
{
  return a->kind == b->kind && (umn_token_litncmp(lexer, a, lexer->data + b->begin, b->length) == 0);
}

inline int umn_token_issymbol(const umn_Lexer *lexer, const umn_Token *token, const char *literal)
{
  return (token->kind & (~UMN_K__RESERVED__)) == UMN_KSYMBOL && (umn_token_litcmp(lexer, token, literal) == 0);
}

int umn_token__kind_to_str(const umn_Token *token, char *dest, const size_t size)
{
  umn_sb_t sb = {.capacity = size, .items = dest};
  int sb_err = 0;

  /* convert to the string builder approach  */

  /* so what this thing does is to return ERR:RESERVER_1:KIND(enc ... ) */
  if (token->kind & UMN_KERR)
    sb_err = umn_sb_pushs(&sb, "ERR:");

  /* TODO: the reserved bits are flags so treat it as such, i.e. multiple hits */
  if (token->kind & UMN_K__RESERVED__)
  {
    umn_Token_Kind v = (token->kind & UMN_K__RESERVED__);

    for (int r = 0; r < 8; r++)
    {
      if ((v & (UMN_K__1 << r)))
      {
        sb_err = umn_sb_pushf(&sb, "R%d:", r + 1);
        break;
      }
    }
  }

  char *kind = NULL;
  umn_Token_Kind v = (token->kind & (~UMN_KERR ^ UMN_K__RESERVED__));
  if (UMN_KEOF == v)
    kind = "UMN_KEOF";
  else if (UMN_KLITERAL == v)
    kind = "UMN_KLITERAL";
  else if (UMN_KSTRING == v)
    kind = "UMN_KSTRING";
  else if (UMN_KINTEGER == v)
    kind = "UMN_KINTEGER";
  else if (UMN_KFRACTION == v)
    kind = "UMN_KFRACTION";
  else if (UMN_KSYMBOL == v)
    kind = "UMN_KSYMBOL";

  if (kind)
    sb_err = umn_sb_pushs(&sb, kind);
  else
    sb_err = umn_sb_pushf(&sb, "unknown(%#lx)", (token->kind & (~UMN_KERR)));

  if (token->kind & UMN_KNUMERIC && (token->encoding & 0xFF))
    sb_err = umn_sb_pushf(&sb, "(%c)", token->encoding);

  assert(sb_err == 0); /* assert that the string builder did not fail and stuff ... */
  return sb.count;
}

void umn_token_print_(const umn_Lexer *lexer, const umn_Token *token)
{
  char value[128] = {0}, kind[128] = {0};
  umn_token__kind_to_str(token, kind, sizeof(kind));
  strncpy(value, lexer->data + token->begin, token->length);

  printf("umn_Token { %s, .begin=%zu, .length=%d, value=\"%s\"}\n",
         kind, token->begin, token->length, value);
}

#define umn_token_print(lexer, token) printf("%s:%d", __FILE__, __LINE__), umn_token_print_(lexer, token)

void umn_token_print_error(const umn_Lexer *lexer, const umn_Token *token)
{
  char buffer[512] = {0};
  assert(sizeof(buffer) > ((token->begin - token->line_offset) + token->length));

  puts(lexer->data + (token->begin - token->line_offset));

  for (size_t i = 0; i < token->line_offset; i++)
    putchar(' ');

  for (size_t i = 0; i < token->length; i++)
    putchar('^');

  putchar('\n');
}

#endif
#endif
