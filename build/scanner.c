#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "error.h"
#include "scanner.h"

int hasInitIndent = 0;
int hasFinalDedent = 0;
int emitDedents = 0;
int emitFakeNewline = 0;
int dupTimes = 0;
int EMIT_LAZY_BANG = 0;
int EXPECT_UNITNAME_NEXT = 0;
int FORCE_STRING = 0;

int EMIT_FAKE_OPAREN = 0;
int EMIT_FAKE_PLUS = 0;

struct Token lastTok;

struct Token* tok(TokenType type, char* start, int len, int line, int display_line, int col, char* source_name) {
  struct Token* tok = (struct Token*)malloc(sizeof(struct Token));
  *tok = (struct Token){ .type = type, .start = start, .sobj = { .len = len, .line = line, .display_line = display_line, .col = col, .source_name = source_name } };
  lastTok = *tok;
  return tok;
}

void dupToken(){
  dupTimes ++;
}

char peek() {
  return *scanner.current;
}

char peektwo() {
  return *(scanner.current + 1);
}

char next() {
  char c = *scanner.current;
  scanner.current ++;
  scanner.col ++;
  return c;
}

void skip_ws() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ': case '\r': case '\t':
        next();
        break;

      case '#': {
        char p;
        while((p = peek()) != '\n' && p != '\0') next();
        break;
      }

      default:
        return;
    }
  }
}

char is_valid_id_start(char c){
  return c == '_' || isalpha(c) || c == '.' || c == '?' || c == '\'';
}

struct Token* scan(void) {
  if(EMIT_FAKE_OPAREN){
    EMIT_FAKE_OPAREN --;
    return tok(T_LPAR, scanner.current - 2, 2, scanner.line, scanner.display_line, scanner.col - 2, scanner.current_unit_name);
  }

  if(EMIT_FAKE_PLUS){
    EMIT_FAKE_PLUS --;
    return tok(T_PLUS, scanner.current - 2, 2, scanner.line, scanner.display_line, scanner.col - 2, scanner.current_unit_name);
  }
  
  if(FORCE_STRING){
    FORCE_STRING --;
    goto do_string;
  }
  
  if(EMIT_LAZY_BANG){
    EMIT_LAZY_BANG = 0;
    next();
    return tok(T_LAZY_BANG, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
  }
  
  if(!hasInitIndent) {
    hasInitIndent = 1;
    return tok(T_INDENT, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
  }

  if(emitFakeNewline) {
    emitFakeNewline --;
    return tok(T_SEP, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
  }

  if(emitDedents) {
    emitDedents --;
    if(!emitDedents) emitFakeNewline ++;

    return tok(T_DEDENT, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
  }

  if(dupTimes) {
    dupTimes --;
    struct Token* tok = (struct Token*)malloc(sizeof(struct Token));
    memcpy(tok, &lastTok, sizeof(struct Token));
    return tok;
  }

  skip_ws();
  
  char c = peek();

  char is_symbol = c == ':' && is_valid_id_start(peektwo());
  if (is_valid_id_start(c) || is_symbol) {
    char* current = scanner.current;
    int len = 0;
    do {
      next();
      c = peek();
      len ++;
    } while (is_valid_id_start(c) || isdigit(c));
    if(len == 2 && strncmp(current, "if", 2) == 0) {
      return tok(T_IF, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 2 && strncmp(current, "or", 2) == 0) {
      return tok(T_ELSE, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 2 && strncmp(current, "do", 2) == 0) {
      return tok(T_WHILE, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 2 && strncmp(current, "my", 2) == 0) {
      return tok(T_MY, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 4 && strncmp(current, "None", 4) == 0) {
      return tok(T_NONE, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 4 && strncmp(current, "defn", 4) == 0) {
      return tok(T_DEF, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 4 && strncmp(current, "lazy", 4) == 0) {
      return tok(T_LAZY, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    } else if(len == 4 && strncmp(current, "unit", 4) == 0){
      scanner.display_line = 1;
      EXPECT_UNITNAME_NEXT = 1;
      return tok(T_UNIT, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
    }

    // context-sensitive lexing
    if(*scanner.current == '!'){
      EMIT_LAZY_BANG = 1;
    }
    if(EXPECT_UNITNAME_NEXT){
      EXPECT_UNITNAME_NEXT = 0;
      free(scanner.current_unit_name);
      scanner.current_unit_name = (char*)malloc((len + 1) * sizeof(char));
      strncpy(scanner.current_unit_name, current, len);
      scanner.current_unit_name[len] = '\0';
    }
    return tok(is_symbol ? T_SYM : T_ID, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
  }

  if (isdigit(c)) {
    char* current = scanner.current;
    int len = 0;
    while (isdigit(c)) {
      next();
      c = peek();
      len ++;
    }
    return tok(T_NUM, current, len, scanner.line, scanner.display_line, scanner.col - len, scanner.current_unit_name);
  }

#define onechartok(x) struct Token* token = tok(x, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name); next(); return token

  switch (c) {
    case '\n': {
      struct Token* token = tok(T_SEP, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);

      while(peek() == '\n') {
        next();
        scanner.line ++;
        scanner.display_line ++;
        scanner.col = 1;
      }

      skip_ws();
      if(scanner.indentlevel[scanner.indentlen - 1] < scanner.col - 1){
        scanner.indentlevel[scanner.indentlen] = scanner.col - 1;

        scanner.indentlen ++;
        if(scanner.indentlen >= scanner.indentcap) {
          scanner.indentcap *= 2;
          scanner.indentlevel = (int*)realloc(scanner.indentlevel, scanner.indentcap * sizeof(int));
        }
        return tok(T_INDENT, scanner.current - 1, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
      } else if (scanner.indentlevel[scanner.indentlen - 1] > scanner.col - 1) {
        do {
          scanner.indentlen --;
          emitDedents ++;
        } while (scanner.indentlevel[scanner.indentlen - 1] > scanner.col - 1);

        if(scanner.indentlevel[scanner.indentlen - 1] != scanner.col - 1) {
          internal_error("LexingError: Unexpected unindent!");
        }

        emitDedents --;
        if(!emitDedents) emitFakeNewline ++;
        return tok(T_DEDENT, scanner.current - 1, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
      }

      return token;
    }

    case ',': {
      onechartok(T_COMMA);
    }

    case ';': {
      onechartok(T_SEMICOLON);
    }

    case '(': {
      if(peektwo() == ')') {
        struct Token* token = tok(T_NONE, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return token;
      }
      onechartok(T_LPAR);
    }

    case '&': {
      onechartok(T_AND);
    }

    case '|': {
      onechartok(T_OR);
    }

    case ')': {
      onechartok(T_RPAR);
    }

    case '=': {
      if(peektwo() == '=') {
        struct Token* token = tok(T_EQEQ, scanner.current, 2, scanner.line, scanner.display_line, scanner.col,  scanner.current_unit_name);
        next(); next();
        return token;
      }
      onechartok(T_EQ);
    }
    case '!': {
      if(peektwo() == '=') {
        struct Token* token = tok(T_NE, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return token;
      }
      onechartok(T_NOT);
    }
    case '>': {
      if(peektwo() == '=') {
        struct Token* token = tok(T_GTEQ, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return token;
      }
      onechartok(T_GT);
    }
    case '<': {
      if(peektwo() == '=') {
        struct Token* token = tok(T_LTEQ, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return token;
      }
      onechartok(T_LT);
    }

    case ':': {
      onechartok(T_COLON);
    }

    case '+': {
      onechartok(T_PLUS);
    }
    case '-': {
      onechartok(T_MINUS);
    }
    case '/': {
      onechartok(T_SLASH);
    }
    case '*': {
      onechartok(T_STAR);
    }

    case '"': {
      next();
      do_string: ;
      char* current = scanner.current;
      int len = 0;
      while (peek() != '"') {
        if (peek() == '\\') {
          next();
          len ++;

          if(peek() == '{' && peektwo() == '{'){
            next(); next(); len += 2;
          }
        }
        
        if (peek() == '{' && peektwo() == '{'){
          break;
        }
        
        if(peek() == '\n') {
          next();
          scanner.col = 1;
          scanner.line ++;
          scanner.display_line ++;
        } else next();
        
        len ++;
      }
      if(peek() == '"') next(); // closing quote
      return tok(T_STR, current, len, scanner.line, scanner.display_line, scanner.col - len - 1, scanner.current_unit_name);
    }

    case '{': {
      if(peektwo() == '{'){
        EMIT_FAKE_OPAREN ++;
        struct Token* t = tok(T_PLUS, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return t;
      }
    }

    case '}': {
      if(peektwo() == '}'){
        EMIT_FAKE_PLUS ++;
        FORCE_STRING ++;
        struct Token* t = tok(T_RPAR, scanner.current, 2, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
        next(); next();
        return t;
      }
    }

    case '\0':
    case EOF:
      if(!hasFinalDedent) {
        hasFinalDedent = 1;
        return tok(T_DEDENT, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);
      }
      return tok(T_EOF, scanner.current, 1, scanner.line, scanner.display_line, scanner.col, scanner.current_unit_name);

    default:
      general_error((struct sObj){ .line = scanner.line, .display_line = scanner.display_line, .col = scanner.col, .len = 1, .source_name = scanner.current_unit_name}, "LexingError", "This is a typo", "Unknown token \"%c\"", c);
  }

#undef onechartok

}

void initScanner(char* source, char* sourcename) {
  scanner.source = source;
  scanner.current = source;
  scanner.line = 1;
  scanner.display_line = 1;
  scanner.col = 1;

  scanner.indentlevel = malloc(5 * sizeof(int));
  scanner.indentlevel[0] = 0;

  scanner.indentcap = 5;
  scanner.indentlen = 1;

  scanner.current_unit_name = sourcename;
}

void cleanupScanner() {
  free(scanner.indentlevel);
}

void printTokType(TokenType typ) {
  switch(typ) {
    case T_SEP:
      printf("T_SEP"); break;
    case T_ID:
      printf("T_ID"); break;
    case T_SYM:
      printf("T_SYM"); break;
    case T_EOF:
      printf("T_EOF"); break;

    case T_LPAR:
      printf("T_LPAR"); break;
    case T_RPAR:
      printf("T_RPAR"); break;
    case T_SEMICOLON:
      printf("T_SEMICOLON"); break;
    case T_COMMA:
      printf("T_COMMA"); break;
    case T_EQ:
      printf("T_EQ"); break;
    case T_COLON:
      printf("T_COLON"); break;
      
    case T_EQEQ:
      printf("T_EQEQ"); break;
    case T_NE:
      printf("T_NE"); break;
    case T_LT:
      printf("T_LT"); break;
    case T_LTEQ:
      printf("T_LTEQ"); break;
    case T_GT:
      printf("T_GT"); break;
    case T_GTEQ:
      printf("T_GTEQ"); break;
    case T_NOT:
      printf("T_NOT"); break;

    case T_LAZY_BANG:
      printf("T_LAZY_BANG"); break;

    case T_PLUS:
      printf("T_PLUS"); break;
    case T_MINUS:
      printf("T_MINUS"); break;
    case T_SLASH:
      printf("T_SLASH"); break;
    case T_STAR:
      printf("T_STAR"); break;

    case T_IF:
      printf("T_IF"); break;
    case T_ELSE:
      printf("T_ELSE"); break;
    case T_WHILE:
      printf("T_WHILE"); break;
    case T_DEF:
      printf("T_DEF"); break;
    case T_LAZY:
      printf("T_LAZY"); break;
    case T_MY:
      printf("T_MY"); break;
    case T_NONE:
      printf("T_NONE"); break;
    case T_UNIT:
      printf("T_UNIT"); break;
    
    case T_AND:
      printf("T_AND"); break;
    case T_OR:
      printf("T_OR"); break;

    case T_STR:
      printf("T_STR"); break;
    case T_NUM:
      printf("T_NUM"); break;

    case T_INDENT:
      printf("T_INDENT"); break;
    case T_DEDENT:
      printf("T_DEDENT"); break;
  }
}

void printToken(struct Token* tok) {
  printf("TOK{ \"%.*s\", type= ", (int)tok->sobj.len, tok->start);
  printTokType(tok->type);
  printf(", line= %zu, col= %zu }\n", tok->sobj.line, tok->sobj.col);
}