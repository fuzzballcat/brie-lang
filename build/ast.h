#ifndef ast_h
#define ast_h

#include "scanner.h"

struct ArgumentObject {
  struct ExprNode** arguments;
  int arity;
};

struct ExprNode {
  int lineno;

  enum {
    FunctionCall,
    StringExpr,
    IdentifierExpr,
    AssignmentExpr,
    NumExpr,
    BinopExpr,
    NoneExpr,
    UnaryExpr
  } type;

  union {
    struct {
      struct ExprNode* callee;
      struct ArgumentObject** arguments;
      int num_of_argsets;
      int force; // parapp lazy force
    } function_call;

    struct {
      struct Token* str;
    } string_expr;

    struct {
      struct Token* id;
    } id_expr;

    struct {
      struct ExprNode** a_s;
      int a_s_len;
      struct ExprNode* b;
    } assignment_expr;

    struct {
      struct Token* num;
    } num_expr;

    struct {
      char type;
      struct ExprNode* left;
      struct ExprNode* right;
    } binop_expr;

    struct {
      char type;
      struct ExprNode* expr;
    } unary_expr;
  } as;
};

struct StmtNode {
  int lineno;

  enum {
    ExprStmt,
    StmtList,
    IfStmt,
    IfElseStmt,
    WhileStmt,
    FnDecl,
    MyArg
  } type;

  union {
    struct {
      struct ExprNode* expr;
    } expr_stmt;

    struct {
      struct StmtNode* stmts;
      int len;
    } stmt_list;

    struct {
      struct ExprNode* cond;
      struct StmtNode* stmts;
    } if_stmt;

    struct {
      struct ExprNode* cond;
      struct StmtNode* stmts;
      struct StmtNode* elsestmts;
    } ifelse_stmt;

    struct {
      struct ExprNode* cond;
      struct StmtNode* stmts;
    } while_stmt;

    struct {
      struct Token name;
      struct StmtNode* stmts;
      int is_lazy;
    } fn_decl;

    struct {
      struct Token* name;
      int num;
    } my_arg;
  } as;
};

void printStmtNode(struct StmtNode* stmt);
void printExprNode(struct ExprNode* expr);

int is_assignable(struct ExprNode* expr);

#endif