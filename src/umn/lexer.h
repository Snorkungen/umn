#ifndef UMN_LEXER_H
#define UMN_LEXER_H

/* TODO: wrap theese in something so that windows can compile this without any major issues */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  umn_Enc_Integer_Dec = '\0',
  umn_Enc_Integer_Hexadec = 'x',
  umn_Enc_Integer_Binary = 'b',
  umn_Enc_Integer_Octal = '0',
  umn_Enc_Fraction_E = 'E',

  umn_Enc_String_1 = '\'',
  umn_Enc_String_2 = '\"',
} umn_Token_Encoding;

typedef struct
{
  uint32_t flags;
  uint32_t data;
} umn_Symbol_Attrs;

typedef struct
{
  const char *s; /* symbol could be casted to (char *) */
  int length;
  umn_Symbol_Attrs attrs;
} umn_Symbol;

typedef uint64_t umn_Token_Kind;
typedef struct
{
  umn_Token_Kind kind;
  uint64_t begin;
  uint32_t length;
  /* how many bytes this is offset from the lines start */
  uint32_t line_offset;
  union
  {
    umn_Symbol_Attrs symbol;
    umn_Token_Encoding encoding;
  } d;
} umn_Token;

/* Should the lexer support strings because the intention was always to support string ... */
typedef struct
{
  size_t position; /* Location of where to read next */
  size_t line_begin;

  size_t symbol_count;
  umn_Symbol *symbols;
  umn_Symbol_Attrs symbol_attrs;

  size_t data_len;
  char const *data;
} umn_Lexer;

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
                            UMN_KSYMBOL = UMN_KLITERAL | 0x80;
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

int umn_token_litncmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal, const size_t litsize);
static int umn_token_litcmp(const umn_Lexer *lexer, const umn_Token *token, const char *literal);

/* compares two token if they equal */
static int umn_token_is(const umn_Lexer *lexer, const umn_Token *a, const umn_Token *b);
/* compares a token with a literal */
static int umn_token_issymbol(const umn_Lexer *lexer, const umn_Token *token, const char *literal);

void umn_token_print(const umn_Lexer *lexer, const umn_Token *token);
void umn_token_print_error(const umn_Lexer *lexer, const umn_Token *token);

#ifdef UMN_LEXER_IMPL

int umn_lexer__match_symbol(umn_Lexer *lexer, umn_Token *token);
int umn_lexer_next(umn_Lexer *lexer, umn_Token *token)
{
  memset(token, 0, sizeof(*token));
  size_t separation_pos = (lexer->data_len);
  wchar_t current;

  if (lexer->position > lexer->data_len || lexer->data == NULL)
  {
    token->kind = UMN_KEOF | UMN_KERR;
    return -1;
  }

  for (; lexer->position < lexer->data_len; lexer->position++)
  {
    assert((token->kind & UMN_KERR) == 0);

    current = lexer->data[lexer->position];

    while ((lexer->position + 1) < lexer->data_len && (lexer->data[lexer->position + 1] & 0x80))
    {
      assert(0); /* TODO: handle UTF-8 */
    }

    if (current <= ' ')
    { /* ignore chars that are leq 32 */
      if (token->kind != 0)
        break;

      if (current == '\n')
        lexer->line_begin = lexer->position;

      continue;
    }
    else if (current == umn_Enc_String_1 || current == umn_Enc_String_2) /* UMN_KSTRING */
    {
      if (token->kind != 0)
        break;

      token->d.encoding = (umn_Token_Encoding)current;
      token->kind = UMN_KSTRING;
      token->line_offset = lexer->position - lexer->line_begin;
      token->begin = lexer->position + 1; /* only care about the string contents " contents " */

      int count = 0;
      for (lexer->position += 1; lexer->position < lexer->data_len; lexer->position++)
      {
        if (lexer->data[lexer->position] == current && ((count & 1) == 0))
          break;

        /* branchless so cool, does a comparison actually retur 1 .. */
        count = (int)(lexer->data[lexer->position] == '\\') * (count + 1);
      }

      if (lexer->data[lexer->position] != current)
      {
        token->kind |= UMN_KERR;
        break;
      }
      else
      {
        token->length = (lexer->position) - token->begin;
        lexer->position++;
        return 0;
      }
    }
    else if (token->kind == UMN_KINTEGER)
    {
      int __start_of_integer_enc__ = token->d.encoding == umn_Enc_Integer_Dec && lexer->data[token->begin] == '0' && (token->begin + 1) == lexer->position;

      if (token->d.encoding == umn_Enc_Integer_Binary && (current == '1' || current == '0'))
        continue;
      else if (token->d.encoding == umn_Enc_Integer_Hexadec && isxdigit(current))
        continue;
      else if (token->d.encoding == umn_Enc_Integer_Octal && (isdigit(current) && (current - '0' <= 7)))
        continue;
      else if (token->d.encoding == umn_Enc_Integer_Dec && current == '.')
      {
        token->kind = UMN_KFRACTION;
        separation_pos = lexer->position;
        continue;
      }
      else if (__start_of_integer_enc__ && tolower(current) == umn_Enc_Integer_Binary)
      {
        token->d.encoding = umn_Enc_Integer_Binary;
        separation_pos = lexer->position;
        continue;
      }
      else if (__start_of_integer_enc__ && tolower(current) == umn_Enc_Integer_Hexadec)
      {
        token->d.encoding = umn_Enc_Integer_Hexadec;
        separation_pos = lexer->position;
        continue;
      }
      else if (__start_of_integer_enc__ && isdigit(current) && (current - '0') <= 7)
      {
        token->d.encoding = umn_Enc_Integer_Octal;
        continue;
      }
      else if (__start_of_integer_enc__ && isdigit(current))
      {
        token->kind |= UMN_KERR;
        lexer->position++;
        break;
      }
      else if (token->d.encoding == umn_Enc_Integer_Dec && isdigit(current))
        continue;
      else
        break;
    }
    else if (token->kind == UMN_KFRACTION)
    {
      if (token->d.encoding == umn_Enc_Fraction_E && (lexer->position == separation_pos + 1) && (current == '-' || current == '+'))
        continue;
      else if (isdigit(current))
        continue;
      else if (token->d.encoding == umn_Enc_Integer_Dec && lexer->position > (separation_pos + 1) && tolower(current) == 'e')
      {
        token->d.encoding = umn_Enc_Fraction_E;
        separation_pos = lexer->position;
        continue;
      }
      else
        break;
    }

    /* assging something atleast */
    if (token->kind == 0)
    {
      token->line_offset = lexer->position - lexer->line_begin;
      token->begin = lexer->position;
      token->kind = isdigit(current) ? UMN_KINTEGER : UMN_KLITERAL;
    }
  }

  if (token->kind == 0)
  {
    return 0; /* this means that there is no more data */
  }

  if (separation_pos == (lexer->position - 1))
  {
    token->kind |= UMN_KERR_SEP;
    lexer->position++;
  }

  token->length = lexer->position - token->begin;

  if ((token->kind & UMN_KERR))
  {
    return -1;
  }

  assert(token->length > 0 || token->kind == UMN_KSTRING);

  if (UMN_KLITERAL == token->kind)
    return umn_lexer__match_symbol(lexer, token);

  return 0;
}

int umn_lexer_peek(const umn_Lexer *lexer, umn_Token *token)
{
  umn_Lexer tmp;
  memcpy(&tmp, lexer, sizeof(tmp));
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

int umn_lexer__match_symbol(umn_Lexer *lexer, umn_Token *token)
{
  if (UMN_KLITERAL != token->kind || token->length == 0)
    return -1;

  const umn_Symbol *symbol, *best_symbol = NULL;
  int is_strong;

  for (int si = 0; si < lexer->symbol_count; si++)
    lexer->symbols[si].length = strlen(lexer->symbols[si].s);

  for (int i = 0; i < token->length; i++)
  { /* outer loop itterate through the entire literal */
    /* itterate over the symbols */

    for (int j = 0; j < lexer->symbol_count; j++)
    {
      symbol = lexer->symbols + j;

      if (symbol->length == 0 || symbol->length > (token->length - i) || (best_symbol != NULL && symbol->length < best_symbol->length))
        continue;

      is_strong = symbol->attrs.flags == 0 || (token->length == symbol->length) || (symbol->attrs.flags & lexer->symbol_attrs.flags);

      if (!is_strong || strncmp(symbol->s, lexer->data + (token->begin + i), symbol->length) != 0)
        continue;

      best_symbol = symbol;
    }

    if (best_symbol == NULL)
      continue;

    if (i > 0)
    { /* Check for weak symbol */
      best_symbol = NULL;

      for (int j = 0; j < lexer->symbol_count; j++)
      {
        symbol = lexer->symbols + j;

        if (symbol->length > i || symbol->length == 0 || (best_symbol != NULL && symbol->length < best_symbol->length))
          continue;
        if (strncmp(symbol->s, lexer->data + token->begin, symbol->length))
          continue;

        best_symbol = symbol;
      }
    }

    if (best_symbol == NULL)
    {
      token->length = i;
      lexer->position = token->begin + token->length;
    }
    else
    {
      token->kind = UMN_KSYMBOL;
      token->length = best_symbol->length;

      lexer->position = token->begin + token->length;
      memcpy(&token->d.symbol, &best_symbol->attrs, sizeof(token->d.symbol));
    }

    return 0;
  }

  return 0;
}

int64_t umn_token_readi(const umn_Lexer *lexer, const umn_Token *token)
{
  char const *s_beg = lexer->data + token->begin;
  char *s_end = (char *)s_beg + token->length;

  switch (token->d.encoding)
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

double umn_token_readf(const umn_Lexer *lexer, const umn_Token *token)
{
  char const *s_beg = lexer->data + token->begin;
  char *s_end = (char *)s_beg + token->length;

  return strtod(s_beg, &s_end);
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

void umn_token_print(const umn_Lexer *lexer, const umn_Token *token)
{
  char value[128] = {0}, kind[128] = {0};

  {

#define __str_macro__(KIND)                                    \
  if (KIND == (token->kind & (~UMN_KERR ^ UMN_K__RESERVED__))) \
    strncpy(kind, #KIND, sizeof(kind));

    __str_macro__(UMN_KEOF);
    __str_macro__(UMN_KLITERAL);
    __str_macro__(UMN_KSTRING);
    __str_macro__(UMN_KNUMERIC);
    __str_macro__(UMN_KINTEGER);
    __str_macro__(UMN_KFRACTION);
    __str_macro__(UMN_KSYMBOL);
#undef __str_macro__

    if (*kind == 0)
    {
      snprintf(kind, sizeof(kind), "unknown(%#lx)", (token->kind & (~UMN_KERR)));
    }

    if (token->kind & UMN_KNUMERIC)
    {
      char tmp[sizeof(kind) + 16] = {0};
      if (token->d.encoding == 0)
        snprintf(tmp, sizeof(tmp), "%s(\\0)", kind);
      else
        snprintf(tmp, sizeof(tmp), "%s(%c)", kind, (char)(token->d.encoding & 0xFF));
      strncpy(kind, tmp, sizeof(kind));
    }

    if (token->kind & UMN_KERR)
    {
      char tmp[sizeof(kind) + 16] = {0};
      snprintf(tmp, sizeof(tmp), "ERR(%s)", kind);
      strncpy(kind, tmp, sizeof(kind));
    }
  }

  strncpy(value, lexer->data + token->begin, token->length);
  printf("umn_Token { %s, .begin=%zu, .length=%d, value=\"%s\"}\n",
         kind, token->begin, token->length, value);
}

void umn_token_print_error(const umn_Lexer *lexer, const umn_Token *token)
{
  char buffer[512] = {0};
  assert(sizeof(buffer) > ((token->begin - token->line_offset) + token->length));

  puts(lexer->data + (token->begin - token->line_offset));

  for (int i = 0; i < token->line_offset; i++)
    putchar(' ');

  for (int i = 0; i < token->length; i++)
    putchar('^');

  putchar('\n');
}

#endif
#endif
