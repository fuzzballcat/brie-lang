#include <stdio.h>

#include "ast.h"
#include "scanner.h"

void printExprNode(struct ExprNode* node) {
  switch(node->type) {
    case NoneExpr: printf("None"); break;
    case FunctionCall:
      printExprNode(node->as.function_call.callee);
      if(node->as.function_call.force) printf("!");
      printf(" (");
      for(int i = 0; i < node->as.function_call.num_of_argsets; i ++) {
        for(int q = 0; q < node->as.function_call.arguments[i]->arity; q ++){
          printExprNode(node->as.function_call.arguments[i]->arguments[q]);
          if(q+1 < node->as.function_call.arguments[i]->arity) printf(", ");
        }
        if(i+1 < node->as.function_call.num_of_argsets) printf("; ");
      }
      printf(")");
      break;

    case StringExpr:
      printf("\"%s\"", node->as.string_expr.str);
      break;

    case NumExpr:
      printf("%.*s", (int)node->as.num_expr.num->sobj.len, node->as.num_expr.num->start); 
      break;
    
    case AssignmentExpr:
      for(int i = 0; i < node->as.assignment_expr.a_s_len; i ++){
        printExprNode(node->as.assignment_expr.a_s[i]);
        if(i+1 < node->as.assignment_expr.a_s_len) printf(", ");
      }
      printf(" = ");
      for(int i = 0; i < node->as.assignment_expr.b_s_len; i ++){
        printExprNode(node->as.assignment_expr.b_s[i]);
        if(i+1 < node->as.assignment_expr.b_s_len) printf(", ");
      }
      break;
    
    case IdentifierExpr:
      printf("%s", node->as.id_expr.id);
      break;

    case BinopExpr:
      printExprNode(node->as.binop_expr.left);
      printf(" %c ", node->as.binop_expr.type);
      printExprNode(node->as.binop_expr.right);
      break;
    case UnaryExpr:
      printf("%c(", node->as.unary_expr.type);
      printExprNode(node->as.unary_expr.expr);
      printf(")");
      break;
  }
}

void printStmtNode(struct StmtNode* node) {
  switch(node->type) {
    case ExprStmt:
      printExprNode(node->as.expr_stmt.expr);
      printf("\n");
      break;

    case StmtList:
      for(int i = 0; i < node->as.stmt_list.len; i ++) {
        printStmtNode(&node->as.stmt_list.stmts[i]);
      }
      break;

    case IfStmt:
      printf("if ");
      printExprNode(node->as.if_stmt.cond);
      printf(": {\n");
      printStmtNode(node->as.if_stmt.stmts);
      printf("}\n");
      break;
    
    case IfElseStmt:
      printf("if ");
      printExprNode(node->as.ifelse_stmt.cond);
      printf(": {\n");
      printStmtNode(node->as.ifelse_stmt.stmts);
      printf("} else: {\n");
      printStmtNode(node->as.ifelse_stmt.elsestmts);
      printf("}\n");
      break;

    case WhileStmt:
      printf("while ");
      printExprNode(node->as.while_stmt.cond);
      printf(": {\n");
      printStmtNode(node->as.while_stmt.stmts);
      printf("}\n");
      break;

    case FnDecl:
      if(node->as.fn_decl.is_lazy) printf("lazy ");
      else printf("def ");
      printf("%s", node->as.fn_decl.name);
      printf(": {\n");
      printStmtNode(node->as.fn_decl.stmts);
      printf("}\n");
      break;

    case MyArg:
      printf("my ");
      for(int i = 0; i < node->as.my_arg.num; i ++){
        printf("%s", node->as.my_arg.name[i]);
        if(i+1 < node->as.my_arg.num) printf("; ");
      }
      printf("\n");
      break;
  }
}


int is_assignable(struct ExprNode* node) {
  switch(node->type) {
    case IdentifierExpr:
      return 1;
    
    default:
      return 0;
  }
}