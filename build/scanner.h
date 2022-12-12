#ifndef scanner_h
#define scanner_h

#include "error.h"

typedef enum {
  T_SEP,
  T_ID,

  T_LPAR,
  T_RPAR,
  T_SEMICOLON,
  T_COMMA,
  T_EQ,
  T_COLON,
  T_EQEQ,
  T_NE,
  T_GT,
  T_GTEQ,
  T_LT,
  T_LTEQ,
  T_NOT,
  T_AND,
  T_OR,

  T_LAZY_BANG,

  T_PLUS,
  T_MINUS,
  T_SLASH,
  T_STAR,

  T_IF,
  T_ELSE,
  T_WHILE,
  T_DEF,
  T_LAZY,
  T_MY,
  T_NONE,

  T_UNIT,

  T_STR,
  T_NUM,

  T_INDENT,
  T_DEDENT,
  
  T_EOF
} TokenType;

struct Token {
  TokenType type;
  char* start;

  struct sObj sobj;
};

struct Token *scan(void);
void initScanner(char*, char*);
void cleanupScanner(void);

void dupToken(void);

void printToken(struct Token*);


struct Scanner {
  char* current;

  int line;
  int display_line;
  int col;

  int* indentlevel;
  int indentcap;
  int indentlen;

  char* current_unit_name;
  char* source;
};

struct Scanner scanner;

#endif