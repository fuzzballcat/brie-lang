#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "scanner.h"

struct StmtNode* parse_block();

struct {
  struct Token* current;
  struct Token* previous;
} parser;

static void error(struct Token*, char*) __attribute__((noreturn));
struct ExprNode *parse_expression();

static void error(struct Token* t, char* msg) { 
  if(t->type == T_INDENT)
    printf("\x1b[33m[line %d] \x1b[31mUnexpected indent:\x1b[0m %s\n", t->line, msg);
  else if(t->type == T_DEDENT)
    printf("\x1b[33m[line %d] \x1b[31mUnexpected unindent:\x1b[0m %s\n", t->line, msg);
  else if(t->type == T_SEP)
    printf("\x1b[33m[line %d] \x1b[31mError at <linebreak>:\x1b[0m %s\n", t->line, msg);
  else 
    printf("\x1b[33m[line %d, column %d] \x1b[31mError at \"%.*s\":\x1b[0m %s\n", t->line, t->col, t->length, t->start, msg);
  exit(1);
}

void advance() {
  parser.previous = parser.current;
  parser.current = scan();
}

void expect(TokenType t, char* msg) {
  advance();
  if(parser.previous->type != t) {
    error(parser.previous, msg);
  }
}

struct ExprNode* parse_atom() {
  int line = parser.current->line;
  advance();
  struct Token* t = parser.previous;
  // printToken(t);
  switch (t->type) {
    case T_SEP: error(t, "Unexpected separator!");
    case T_EOF: error(t, "Unexpected EOF!");
    case T_NONE: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .lineno = line, .type = NoneExpr };
      return expr;
    }
    case T_ID: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .lineno = line, .type = IdentifierExpr, .as.id_expr.id = t };
      return expr;
    }
    case T_STR: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .lineno = line, .type = StringExpr, .as.string_expr.str = t };
      return expr;
    }
    case T_NUM: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .lineno = line, .type = NumExpr, .as.num_expr.num = t };
      return expr;
    }
    case T_LPAR: {
      struct ExprNode* expr = parse_expression();
      expect(T_RPAR, "Expected right parenthesis to end expression!");
      return expr;
    }

    default: error(t, "Unexpected token!");
  }
}

struct ExprNode* parse_unary() {
  int line = parser.current->line;
  struct ExprNode* atom = NULL;
  while(parser.current->type == T_MINUS || parser.current->type == T_NOT) {
    advance();
    struct ExprNode* right = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = UnaryExpr, .as.unary_expr = { .type = parser.previous->type == T_NOT ? '!' : '-', .expr = right }};
  }
  if(atom == NULL) {
    return parse_atom();
  }
  struct ExprNode* ptr = atom;
  while(ptr->as.unary_expr.expr != NULL) {
    ptr = ptr->as.unary_expr.expr;
  }
  ptr->as.unary_expr.expr = parse_atom();
  return atom;
}

struct ExprNode* parse_binop_4() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_unary();
  while(parser.current->type == T_STAR || parser.current->type == T_SLASH) {
    advance();
    char type = parser.previous->type == T_STAR ? '*' : '/';
    struct ExprNode* right = parse_unary();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }

  return atom;
}

struct ExprNode* parse_binop_3() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_binop_4();
  while(parser.current->type == T_PLUS || parser.current->type == T_MINUS) {
    advance();
    char type = parser.previous->type == T_PLUS ? '+' : '-';
    struct ExprNode* right = parse_binop_4();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }

  return atom;
}

struct ExprNode* parse_binop_2() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_binop_3();
  while(parser.current->type == T_GT || parser.current->type == T_GTEQ || parser.current->type == T_LT || parser.current->type == T_LTEQ) {
    char type = parser.current->type == T_GT ? '>' : (parser.current->type == T_GTEQ ? 'g' : (parser.current->type == T_LT ? '<' : 'l'));
    advance();

    struct ExprNode* right = parse_binop_3();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_binop_comp() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_binop_2();
  while(parser.current->type == T_EQEQ || parser.current->type == T_NE) {
    char type = parser.current->type == T_NE ? '!' : '=';
    advance();

    struct ExprNode* right = parse_binop_2();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_binop() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_binop_comp();
  while(parser.current->type == T_AND || parser.current->type == T_OR) {
    char type = parser.current->type == T_AND ? '&' : '|';
    advance();

    struct ExprNode* right = parse_binop_comp();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .lineno = line, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_funcall() {
  int line = parser.current->line;
  struct ExprNode* atom = parse_binop();
  if(!(parser.current->type != T_SEP && parser.current->type != T_EOF && parser.current->type != T_DEDENT && parser.current->type != T_RPAR && parser.current->type != T_COLON && parser.current->type != T_EQ && parser.current->type != T_COMMA) && parser.current->type != T_SEMICOLON) return atom;

  int is_parapp_force = 0;
  if(parser.current->type == T_LAZY_BANG){
    advance();
    is_parapp_force = 1;
  }
  
  // know it is funcall now
  struct ArgumentObject** semiseps = malloc(1 * sizeof(struct ArgumentObject*));
  int semiseps_len = 0, semiseps_cap = 1;
  
  do {
    struct ExprNode** args = malloc(sizeof(struct ExprNode*) * 1);
    int len = 0, cap = 1;

    do {
      if (len >= cap) {
        cap *= 2;
        args = realloc(args, sizeof(struct ExprNode*) * cap);
      }

      args[len] = parse_binop();
      len ++;

      if(parser.current->type == T_COMMA) advance();
      else break;
    } while (1);

    if(semiseps_len >= semiseps_cap){
      semiseps_cap *= 2;
      semiseps = realloc(semiseps, sizeof(struct ArgumentObject*) * semiseps_cap);
    }

    struct ArgumentObject* anode = (struct ArgumentObject*) 
malloc(sizeof(struct ArgumentObject));
    anode->arguments = args;
    anode->arity = len;
      
    semiseps[semiseps_len] = anode;
    semiseps_len++;
    
    if(parser.current->type == T_SEMICOLON) advance();
    else break;
  } while(1);

  struct ExprNode* fcall = (struct ExprNode*)malloc(sizeof(struct ExprNode));
  *fcall = (struct ExprNode){ .lineno = line, .type = FunctionCall, .as.function_call = { .callee = atom, .arguments = semiseps, .num_of_argsets = semiseps_len, .force = is_parapp_force } };
  return fcall;
}

struct ExprNode* parse_expression() {
  struct ExprNode* atom = parse_funcall();
  return atom;
}

struct StmtNode* parse_statement() {
  int line = parser.current->line;

  if (parser.current->type == T_IF) {
    advance();
    struct ExprNode* expr = parse_expression();

    expect(T_COLON, "Expect colon after if-statement!");

    struct StmtNode* stmts = parse_block();

    expect(T_SEP, "Expect separator after if-statement block!");

    if (parser.current->type == T_ELSE) {
      struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
      *stmt = (struct StmtNode){ .lineno = line, .type = IfElseStmt, .as.ifelse_stmt = { .cond = expr, .stmts = stmts }};
      struct StmtNode** elseptr = &stmt->as.ifelse_stmt.elsestmts;
      
      while(parser.current->type == T_ELSE){
        advance();
        if(parser.current->type != T_COLON){
          struct ExprNode* cond = parse_expression();
          expect(T_COLON, "Expect colon after else-if clause!");
          struct StmtNode* elsifstmts = parse_block();
          expect(T_SEP, "Expect separator after else-if-statement block!");

          *elseptr = (struct StmtNode*)malloc(sizeof(struct StmtNode));
          if(parser.current->type == T_ELSE){
            **elseptr = (struct StmtNode){ .lineno = line, .type = IfElseStmt, .as.ifelse_stmt = { .cond = cond, .stmts = elsifstmts }};
            elseptr = &(*elseptr)->as.ifelse_stmt.elsestmts;
          } else {
            **elseptr = (struct StmtNode){ .lineno = line, .type = IfStmt, .as.if_stmt = { .cond = cond, .stmts = elsifstmts}};
            // lnohack
            if(parser.current->type != T_DEDENT && parser.current->type != T_SEP){
              parser.current->type = T_SEP;
              dupToken();
            }
            return stmt;
          }
        } else {
          expect(T_COLON, "Expect colon after else clause!");
          struct StmtNode* elsestmts = parse_block();
          *elseptr = elsestmts;
          // lnohack
          if(parser.current->type != T_DEDENT && parser.current->type != T_SEP){
            parser.current->type = T_SEP;
            dupToken();
          }
          return stmt;
        }
      }
    } else {
      struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
      *stmt = (struct StmtNode){ .lineno = line, .type = IfStmt, .as.if_stmt = { .cond = expr, .stmts = stmts} };

      // lnohack
      if(parser.current->type != T_DEDENT && parser.current->type != T_SEP){
        parser.current->type = T_SEP;
        dupToken();
      }
      return stmt;
    }
  }

  else if (parser.current->type == T_WHILE) {
    advance();
    struct ExprNode* expr = parse_expression();

    expect(T_COLON, "Expect colon after while-statement!");

    struct StmtNode* stmts = parse_block();

    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .lineno = line, .type = WhileStmt, .as.while_stmt = { .cond = expr, .stmts = stmts} };
    return stmt;
  }

  else if (parser.current->type == T_DEF || parser.current->type == T_LAZY) {
    int is_lazy = parser.current->type == T_LAZY;
    advance();

    expect(T_ID, "Function name must be valid identifier!");
    struct Token name = *parser.previous;

    expect(T_COLON, "Expect colon after function declaration!");

    struct StmtNode* stmts = parse_block();

    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .lineno = line, .type = FnDecl, .as.fn_decl = { .name = name, .stmts = stmts, .is_lazy = is_lazy } };
    return stmt;
  }

  else if (parser.current->type == T_MY) {
    advance();

    struct Token* names = (struct Token*) malloc(sizeof(struct Token) * 1);
    int names_size = 1, names_len = 0;

    do {
      if(names_len >= names_size){
        names_size *= 2;
        names = realloc(names, names_size * sizeof(struct Token));
      }
      
      expect(T_ID, "Argument name must be valid identifier!");
      names[names_len] = *parser.previous;
      names_len ++;

      if(parser.current->type == T_SEMICOLON) advance();
      else break;
    } while (1);
    
    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .lineno = line, .type = MyArg, .as.my_arg = { .name = names, .num = names_len } };
    return stmt;
  }

  struct ExprNode* expr = parse_expression();
  if (parser.current->type == T_EQ || parser.current->type == T_COMMA) {
    struct ExprNode** exprs = malloc(2 * sizeof(struct ExprNode*));
    exprs[0] = expr;
    int exprs_top = 1, exprs_size = 2;

    while(parser.current->type == T_COMMA){
      advance();

      exprs[exprs_top] = parse_funcall();
      if(!is_assignable(exprs[exprs_top])) error(parser.current, "Cannot assign to non-assignable value!");
      
      exprs_top ++;
      if(exprs_top >= exprs_size) {
        exprs_size *= 2;
        exprs = realloc(exprs, exprs_size * sizeof(struct ExprNode*));
      }
    }
      
    expect(T_EQ, "Expect equals sign to commence assignment!");
    
    struct ExprNode* value = parse_funcall();
    
    struct ExprNode* assignment = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *assignment = (struct ExprNode){ .lineno = line, .type = AssignmentExpr, .as.assignment_expr = { .a_s = exprs, .a_s_len = exprs_top, .b = value } };

    expr = assignment;
  }

  struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
  *stmt = (struct StmtNode){ .lineno = line, .type = ExprStmt, .as.expr_stmt.expr = expr };
  return stmt;
}

struct StmtNode* parse_block() {
  struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
  *stmt = (struct StmtNode){ .lineno = parser.current->line, .type = StmtList, .as.stmt_list = { .len = 1, .stmts = (struct StmtNode*)malloc(1 * sizeof(struct StmtNode))}};

  expect(T_INDENT, "Expect indent to begin block!");

  do {
    stmt->as.stmt_list.stmts[stmt->as.stmt_list.len - 1] = *parse_statement();

    //printStmtNode(&stmt->as.stmt_list.stmts[stmt->as.stmt_list.len - 1]);

    stmt->as.stmt_list.len ++;
    stmt->as.stmt_list.stmts = (struct StmtNode*)realloc(stmt->as.stmt_list.stmts, stmt->as.stmt_list.len * sizeof(struct StmtNode));

    if(parser.current->type != T_EOF && parser.current->type != T_DEDENT){
      expect(T_SEP, "Expect line separator to end statement.");
    }
  } while(parser.current->type != T_EOF && parser.current->type != T_DEDENT);

  if(parser.current->type != T_EOF) expect(T_DEDENT, "Expect dedent to end block!");

  stmt->as.stmt_list.len --;
  
  return stmt;
}

struct StmtNode* parse() {
  advance();
  return parse_block();
}