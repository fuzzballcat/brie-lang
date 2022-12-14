#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"
#include "error.h"

struct StmtNode* parse_block(char is_toplevel);

struct {
  struct Token* current;
  struct Token* previous;

  char* current_unit;
} parser;

struct ExprNode *parse_expression();

void advance() {
  parser.previous = parser.current;
  parser.current = scan();
}

void expect(TokenType t, char* type, char* hint, char* msg) {
  advance();
  if(parser.previous->type != t) {
    general_error(parser.previous->sobj, type, hint, msg);
  }
}

char* id_format(char* idname){
  if(parser.current_unit == NULL || 
        !(strcmp(idname, "print") && strcmp(idname, "rc") && strcmp(idname, "int") && strcmp(idname, "str") && strcmp(idname, "return") && strcmp(idname, "yields") && strcmp(idname, "return") && strcmp(idname, "slice") && strcmp(idname, "len") && strcmp(idname, "input"))
        ){
    return idname;
  }
  char* idcpy = (char*)malloc((strlen(idname)+1) * sizeof(char));
  strcpy(idcpy, idname);
  
  
  char* end = strtok(idcpy, ".");
  if(end == NULL){
    general_error(parser.previous->sobj, "LexingError", "A name may not be composed exclusively of the character '.'.  Consider evaluating your ability to function sanely.", "Invalid name \"%s\"", idname);
  }

  end = strtok(NULL, ".");
  if(end == NULL){
    char* old_idname = idname;
    idname = (char*)malloc((strlen(parser.current_unit) + 9 + strlen(old_idname) + 1) * sizeof(char));
    strcpy(idname, parser.current_unit);
    strcpy(idname + strlen(parser.current_unit), ".private.");
    strcpy(idname + strlen(parser.current_unit) + 9, old_idname);
    //printf("n:%s\n", idname);
    free(old_idname);
  } else {
    char* prev = end, *prev2 = prev;
    while(end != NULL){
      prev2 = prev;
      prev = end;
      end = strtok(NULL, ".");
    }
    if(strcmp(prev2, "private") == 0){
      general_error(parser.previous->sobj, "LexingError", "Try renaming the submodule.", "Cannot use 'private' as submodule: confusing semantics!");
    }
  }
  
  free(idcpy);

  return idname;
}

struct ExprNode* parse_atom() {
  struct sObj sobj = parser.current->sobj;
  advance();
  struct Token* t = parser.previous;
  // printToken(t);
  switch (t->type) {
    case T_SEP: general_error(t->sobj, "ParsingError", "Expressions cannot run onto multiple lines.", "Unexpected separator!");
    case T_EOF: general_error(t->sobj, "ParsingError", "The expression ended abruptly.", "Unexpected EOF!");
    case T_NONE: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .sobj = sobj, .type = NoneExpr };
      return expr;
    }
    case T_ID: {
      char* idname = (char*)malloc((t->sobj.len+1) * sizeof(char));
      strncpy(idname, t->start, t->sobj.len);
      idname[t->sobj.len] = '\0';

      idname = id_format(idname);
      
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .sobj = sobj, .type = IdentifierExpr, .as.id_expr.id = idname };
      return expr;
    }
    case T_STR: {
      char* str = (char*)malloc((t->sobj.len+1) * sizeof(char));
      strncpy(str, t->start, t->sobj.len);
      str[t->sobj.len] = '\0';
      
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .sobj = sobj, .type = StringExpr, .as.string_expr.str = str };
      return expr;
    }
    case T_NUM: {
      struct ExprNode* expr = (struct ExprNode*)malloc(sizeof(struct ExprNode));
      *expr = (struct ExprNode){ .sobj = sobj, .type = NumExpr, .as.num_expr.num = t };
      return expr;
    }
    case T_LPAR: {
      struct ExprNode* expr = parse_expression();
      expect(T_RPAR, "ParsingError", "There shouldn't be any further expression here.  Check for missing commas or semicolons.", "Expected right parenthesis to end expression!");
      return expr;
    }

    default: general_error(t->sobj, "ParsingError", "A core expression may consist of a numeric literal, a string literal, or an identifier.", "Unexpected token while parsing expression!");
  }
}

struct ExprNode* parse_unary() {
  struct ExprNode* atom = NULL;
  while(parser.current->type == T_MINUS || parser.current->type == T_NOT) {
    advance();
    struct ExprNode* right = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = UnaryExpr, .as.unary_expr = { .type = parser.previous->type == T_NOT ? '!' : '-', .expr = right }};
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
  struct ExprNode* atom = parse_unary();
  while(parser.current->type == T_STAR || parser.current->type == T_SLASH) {
    advance();
    char type = parser.previous->type == T_STAR ? '*' : '/';
    struct ExprNode* right = parse_unary();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }

  return atom;
}

struct ExprNode* parse_binop_3() {
  struct ExprNode* atom = parse_binop_4();
  while(parser.current->type == T_PLUS || parser.current->type == T_MINUS) {
    advance();
    char type = parser.previous->type == T_PLUS ? '+' : '-';
    struct ExprNode* right = parse_binop_4();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }

  return atom;
}

struct ExprNode* parse_binop_2() {
  struct ExprNode* atom = parse_binop_3();
  while(parser.current->type == T_GT || parser.current->type == T_GTEQ || parser.current->type == T_LT || parser.current->type == T_LTEQ) {
    char type = parser.current->type == T_GT ? '>' : (parser.current->type == T_GTEQ ? 'g' : (parser.current->type == T_LT ? '<' : 'l'));
    advance();

    struct ExprNode* right = parse_binop_3();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_binop_comp() {
  struct ExprNode* atom = parse_binop_2();
  while(parser.current->type == T_EQEQ || parser.current->type == T_NE) {
    char type = parser.current->type == T_NE ? '!' : '=';
    advance();

    struct ExprNode* right = parse_binop_2();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_binop() {
  struct ExprNode* atom = parse_binop_comp();
  while(parser.current->type == T_AND || parser.current->type == T_OR) {
    char type = parser.current->type == T_AND ? '&' : '|';
    advance();

    struct ExprNode* right = parse_binop_comp();
    struct ExprNode* left = atom;

    atom = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *atom = (struct ExprNode){ .sobj = parser.current->sobj, .type = BinopExpr, .as.binop_expr = { .type = type, .left = left, .right = right }};
  }
  
  return atom;
}

struct ExprNode* parse_funcall() {
  struct sObj sobj = parser.current->sobj;
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
  *fcall = (struct ExprNode){ .sobj = sobj, .type = FunctionCall, .as.function_call = { .callee = atom, .arguments = semiseps, .num_of_argsets = semiseps_len, .force = is_parapp_force } };
  return fcall;
}

struct ExprNode* parse_expression() {
  struct ExprNode* atom = parse_funcall();
  return atom;
}

struct StmtNode* parse_statement() {
  if (parser.current->type == T_IF) {
    advance();
    // purely for error
    if(parser.current->type == T_COLON) {
      expect(T_SEP, "ParsingError", "An if-statement must consist of the keyword \"if\", an expression, and a colon.", "Expect colon after if-statement!");
    }
    
    struct ExprNode* expr = parse_expression();

    expect(T_COLON, "ParsingError", "An if-statement must consist of the keyword \"if\", an expression, and a colon.", "Expect colon after if-statement!");

    struct StmtNode* stmts = parse_block(0);

    expect(T_SEP, "ParsingError", "Try putting the following statements on a new line.", "Expect separator after if-statement block!");

    if (parser.current->type == T_ELSE) {
      struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
      *stmt = (struct StmtNode){ .sobj = parser.current->sobj, .type = IfElseStmt, .as.ifelse_stmt = { .cond = expr, .stmts = stmts }};
      struct StmtNode** elseptr = &stmt->as.ifelse_stmt.elsestmts;
      
      while(parser.current->type == T_ELSE){
        advance();
        if(parser.current->type != T_COLON){
          struct ExprNode* cond = parse_expression();
          expect(T_COLON, "ParsingError", "An else-if clause consists of the keyword \"or\", an optional expression, and a colon.", "Expect colon after else-if clause!");
          struct StmtNode* elsifstmts = parse_block(0);
          expect(T_SEP, "ParsingError", "Try putting the follownig statements on a new line.", "Expect separator after else-if-statement block!");

          *elseptr = (struct StmtNode*)malloc(sizeof(struct StmtNode));
          if(parser.current->type == T_ELSE){
            **elseptr = (struct StmtNode){ .sobj = parser.current->sobj, .type = IfElseStmt, .as.ifelse_stmt = { .cond = cond, .stmts = elsifstmts }};
            elseptr = &(*elseptr)->as.ifelse_stmt.elsestmts;
          } else {
            **elseptr = (struct StmtNode){ .sobj = parser.current->sobj, .type = IfStmt, .as.if_stmt = { .cond = cond, .stmts = elsifstmts}};
            // lnohack
            if(parser.current->type != T_DEDENT && parser.current->type != T_SEP){
              parser.current->type = T_SEP;
              dupToken();
            }
            return stmt;
          }
        } else {
          expect(T_COLON, "ParsingError", "An else clause consists of the keyword \"or\", an optional expression, and a colon.", "Expect colon after else clause!");
          struct StmtNode* elsestmts = parse_block(0);
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
      *stmt = (struct StmtNode){ .sobj = parser.current->sobj, .type = IfStmt, .as.if_stmt = { .cond = expr, .stmts = stmts} };

      // lnohack
      if(parser.current->type != T_DEDENT && parser.current->type != T_SEP){
        parser.current->type = T_SEP;
        dupToken();
      }
      return stmt;
    }
  }

  else if (parser.current->type == T_WHILE) {
    struct sObj sobj = parser.current->sobj;
    advance();
    // purely for error
    if(parser.current->type == T_COLON) {
      expect(T_SEP, "ParsingError", "A while-statement must consist of the keyword \"while\", an expression, and a colon.", "Expect colon after while-statement!");
    }
    struct ExprNode* expr = parse_expression();

    expect(T_COLON, "ParsingError", "A while statement consists of the keyword \"while\", an expression, and a colon.", "Expect colon after while-statement!");

    struct StmtNode* stmts = parse_block(0);

    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .sobj = sobj, .type = WhileStmt, .as.while_stmt = { .cond = expr, .stmts = stmts} };
    return stmt;
  }

  else if (parser.current->type == T_DEF || parser.current->type == T_LAZY) {
    struct sObj sobj = parser.current->sobj;
    
    int is_lazy = parser.current->type == T_LAZY;
    advance();

    expect(T_ID, "ParsingError", "Identifiers may not start with a number and may contain any alphanumeric character, \"_\" and \"?\".", "Function name must be valid identifier!");
    struct Token name = *parser.previous;
    
    char* idname = (char*)malloc((name.sobj.len+1) * sizeof(char));
    strncpy(idname, name.start, name.sobj.len);
    idname[name.sobj.len] = '\0';

    idname = id_format(idname);

    expect(T_COLON, "ParsingError", "A function declaration consists of the keyword \"defn\", a valid name, and a colon.", "Expect colon after function declaration!");

    struct StmtNode* stmts = parse_block(0);

    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .sobj = sobj, .type = FnDecl, .as.fn_decl = { .name = idname, .stmts = stmts, .is_lazy = is_lazy } };
    return stmt;
  }

  else if (parser.current->type == T_MY) {
    struct sObj sobj = parser.current->sobj;
    advance();

    char** names = (char**) malloc(sizeof(char*) * 1);
    int names_size = 1, names_len = 0;

    do {
      if(names_len >= names_size){
        names_size *= 2;
        names = realloc(names, names_size * sizeof(char*));
      }
      
      expect(T_ID, "ParsingError", "Identifiers may not start with a number and may contain any alphanumeric character, \"_\" and \"?\".", "Argument name must be valid identifier!");
      names[names_len] = (char*)malloc((parser.previous->sobj.len + 1) * sizeof(char));
      strncpy(names[names_len], parser.previous->start, parser.previous->sobj.len);
      names[names_len][parser.previous->sobj.len] = '\0';
      names[names_len] = id_format(names[names_len]);
      
      names_len ++;

      if(parser.current->type == T_SEMICOLON) advance();
      else break;
    } while (1);
    
    struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
    *stmt = (struct StmtNode){ .sobj = sobj, .type = MyArg, .as.my_arg = { .name = names, .num = names_len } };
    return stmt;
  }

  struct sObj sobj = parser.current->sobj;
  struct ExprNode* expr = parse_expression();
  if (parser.current->type == T_EQ || parser.current->type == T_COMMA) {
    sobj = parser.current->sobj;
    struct ExprNode** exprs = malloc(2 * sizeof(struct ExprNode*));
    exprs[0] = expr;
    int exprs_top = 1, exprs_size = 2;

    while(parser.current->type == T_COMMA){
      advance();

      exprs[exprs_top] = parse_expression();
      if(!is_assignable(exprs[exprs_top])) general_error(parser.current->sobj, "AssignmentError", "Only valid identifiers may be assigned to.", "Cannot assign to non-assignable value!");
      
      exprs_top ++;
      if(exprs_top >= exprs_size) {
        exprs_size *= 2;
        exprs = realloc(exprs, exprs_size * sizeof(struct ExprNode*));
      }
    }
      
    expect(T_EQ, "ParsingError", "If you wish to execute multiple statements, do not use a comma; place them on separate lines or use the semicolon sequenced call operator.", "Expect equals sign to commence assignment!");

    
    struct ExprNode* value = parse_expression();
    struct ExprNode** values = malloc(2 * sizeof(struct ExprNode*));
    values[0] = value;
    int values_top = 1, values_size = 2;
    while(parser.current->type == T_COMMA){
      advance();
      values[values_top] = parse_expression();

      values_top++;
      if(values_top >= values_size){
        values_size *= 2;
        values = realloc(values, values_size * sizeof(struct ExprNode*));
      }
    }
    
    struct ExprNode* assignment = (struct ExprNode*)malloc(sizeof(struct ExprNode));
    *assignment = (struct ExprNode){ .sobj = sobj, .type = AssignmentExpr, .as.assignment_expr = { .a_s = exprs, .a_s_len = exprs_top, .b_s = values, .b_s_len = values_top } };

    expr = assignment;
  }

  struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
  *stmt = (struct StmtNode){ .sobj = sobj, .type = ExprStmt, .as.expr_stmt.expr = expr };
  return stmt;
}

struct StmtNode* parse_block(char is_toplevel) {
  struct StmtNode* stmt = (struct StmtNode*)malloc(sizeof(struct StmtNode));
  *stmt = (struct StmtNode){ .sobj = parser.current->sobj, .type = StmtList, .as.stmt_list = { .len = 1, .stmts = (struct StmtNode*)malloc(1 * sizeof(struct StmtNode))}};

  expect(T_INDENT, "ParsingError", "A statement block must begin with an indent.", "Expect indent to begin block!");
  do {
    if (parser.current->type == T_UNIT){
      if(!is_toplevel){
        general_error(parser.current->sobj, "ParsingError", "Place exactly one unit declaration at the top of each module file.", "Unit definition must be in a top-level block!");
      }
      
      advance();
      expect(T_ID, "ParsingError", "A unit name may be any valid identifier, including periods.", "Expect identifier for unit name");
      free(parser.current_unit);
      parser.current_unit = (char*)malloc((parser.previous->sobj.len + 1) * sizeof(char));
      strncpy(parser.current_unit, parser.previous->start, parser.previous->sobj.len);
      parser.current_unit[parser.previous->sobj.len] = '\0';

      if(parser.current->type != T_EOF && parser.current->type != T_DEDENT){
        expect(T_SEP, "ParsingError", "A unit declaration must be followed by a new line.", "Expect line separator to unit declaration.");
      }
    }
    while(parser.current->type == T_SEP) advance();
    if(parser.current->type == T_DEDENT) {
      advance();
      continue;
    }

    if(parser.current->type == T_UNIT) continue;
    
    stmt->as.stmt_list.stmts[stmt->as.stmt_list.len - 1] = *parse_statement();

    //printStmtNode(&stmt->as.stmt_list.stmts[stmt->as.stmt_list.len - 1]);

    stmt->as.stmt_list.len ++;
    stmt->as.stmt_list.stmts = (struct StmtNode*)realloc(stmt->as.stmt_list.stmts, stmt->as.stmt_list.len * sizeof(struct StmtNode));

    if(parser.current->type != T_EOF && parser.current->type != T_DEDENT){
      expect(T_SEP, "ParsingError", "Each statement must be placed on a new line.", "Expect line separator to end statement.");
    }
  } while(parser.current->type != T_EOF && parser.current->type != T_DEDENT);

  if(parser.current->type != T_EOF) expect(T_DEDENT, "ParsingError", "Blocks must begin with an indent and end with an unindent.", "Expect dedent to end block!");

  stmt->as.stmt_list.len --;
  
  return stmt;
}

struct StmtNode* parse() {
  advance();
  parser.current_unit = NULL;

  return parse_block(1);
}