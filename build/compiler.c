#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "bytecode.h"
#include "value.h"
#include "scanner.h"
#include "error.h"

static struct Chunk* currentChunk() {
  return &mainChunk;
}

#define FIRST_BYTE(x) ((unsigned)x & 0xff)
#define SECOND_BYTE(x) ((unsigned)x >> 8)

void initChunk(struct Chunk* chunk){
  chunk->code = (uint8_t*) malloc(1 * sizeof(uint8_t));
  chunk->len = 0;
  chunk->capacity = 1;

  chunk->lines = (struct sObj*) malloc(1 * sizeof(struct sObj));

  chunk->pool = (struct Value*) malloc(1 * sizeof(struct Value));
  chunk->poollen = 0;
  chunk->poolcap = 1;
}

void writeChunk(uint8_t byte, struct sObj sobj) {
  struct Chunk* chunk = currentChunk();
  chunk->code[chunk->len] = byte;
  chunk->lines[chunk->len] = sobj;
  chunk->len += 1;

  if(chunk->len >= chunk->capacity) {
    chunk->capacity *= 2;
    chunk->code = (uint8_t*) realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
    chunk->lines = (struct sObj*) realloc(chunk->lines, chunk->capacity * sizeof(struct sObj));
  }
}

void writeLong(int byte, struct sObj sobj) {
  writeChunk(FIRST_BYTE(byte), sobj);
  writeChunk(SECOND_BYTE(byte), sobj);
}

void writeConstant(struct Value value) {
  struct Chunk* chunk = currentChunk();
  chunk->pool[chunk->poollen] = value;
  chunk->poollen ++;
  if(chunk->poollen >= chunk->poolcap) {
    chunk->poolcap *= 2;
    chunk->pool = (struct Value*) realloc(chunk->pool, chunk->poolcap * sizeof(struct Value));
  }
}

void exprBytecode(struct ExprNode* node) {
  switch(node->type) {
    case NoneExpr:
      writeChunk(OP_NONE, node->sobj);
      break;
    case FunctionCall:
      for (int seq_times = 0; seq_times < node->as.function_call.num_of_argsets; seq_times ++){
        writeChunk(OP_STORESTACKBASE, node->sobj);
        for (int i = node->as.function_call.arguments[seq_times]->arity - 1; i >= 0; i --) {
          exprBytecode(node->as.function_call.arguments[seq_times]->arguments[i]);
        }
        exprBytecode(node->as.function_call.callee);
        writeConstant(NUM_VALUE(node->as.function_call.arguments[seq_times]->arity));
        writeChunk(OP_CALL, node->sobj);
        
        writeChunk(node->as.function_call.force, node->sobj);
        
        writeLong(currentChunk()->poollen - 1, node->sobj);  
      }
      break;
    case StringExpr: {
      int len = strlen(node->as.string_expr.str);
      char* str = malloc(len + 1);
      int i = 0;
      for(char* ch = node->as.string_expr.str; ch < node->as.string_expr.str + len; ch ++, i ++){
        if(*ch == '\\'){
          ch ++;
          switch(*ch){
            case 'n':
              str[i] = '\n';
              break;
            case 't':
              str[i] = '\t';
              break;
            case 'r':
              str[i] = '\r';
              break;
            default:
              general_error(node->sobj, "LexingError", "Valid escape characters are \\n, \\t, and \\r.", "Invalid string escape character %c!", *ch);
              break;
          }
        } else {
          str[i] = *ch;
        }
      }
      str[i] = '\0';
      str = realloc(str, (i + 1) * sizeof(char));
      writeConstant(STRING_VALUE(str));
      writeChunk(OP_CONSTANT, node->sobj);
      writeLong(currentChunk()->poollen - 1, node->sobj);
      break;
    }
    case IdentifierExpr: {
      char* str = node->as.id_expr.id;

      if(strcmp(str, "print") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "rc") == 0){
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj); 
      }
      
      else if(strcmp(str, "str") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "int") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "return") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "yields") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "slice") == 0){
       writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "len") == 0){
       writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else if(strcmp(str, "input") == 0) {
        writeConstant(NATIVEFN_VALUE(str));
        writeChunk(OP_CONSTANT, node->sobj);
      }

      else {
        writeConstant(STRING_VALUE(str));
        writeChunk(OP_LOAD, node->sobj);
      }

      writeLong(currentChunk()->poollen - 1, node->sobj);
      break;
    }
    case NumExpr: {
      int len = node->as.num_expr.num->sobj.len;
      double number = 0;
      for(int digit = 1, i = len - 1; i >= 0; i --, digit *= 10) {
        number += digit * (node->as.num_expr.num->start[i] - '0');
      }
      writeConstant(NUM_VALUE(number));
      writeChunk(OP_CONSTANT, node->sobj);
      writeLong(currentChunk()->poollen - 1, node->sobj);
      break;
    }
    case AssignmentExpr: {
      for(int i = node->as.assignment_expr.b_s_len - 1; i >= 0; i --){
        exprBytecode(node->as.assignment_expr.b_s[i]);
      }

      for(int i = 0; i < node->as.assignment_expr.a_s_len; i ++){
        char* str = node->as.assignment_expr.a_s[i]->as.id_expr.id;
        
        writeConstant(STRING_VALUE(str));
        writeChunk(OP_ASSIGN, node->sobj);
        writeLong(currentChunk()->poollen - 1, node->sobj);
      }
      
      break;
    }
    case BinopExpr: {
      exprBytecode(node->as.binop_expr.right);
      exprBytecode(node->as.binop_expr.left);
      switch(node->as.binop_expr.type) {
        case '+':
          writeChunk(BINARY_ADD, node->sobj); break;
        case '-':
          writeChunk(BINARY_SUB, node->sobj); break;
        case '*':
          writeChunk(BINARY_MUL, node->sobj); break;
        case '/':
          writeChunk(BINARY_DIV, node->sobj); break;
        case '=':
          writeChunk(BINARY_EQEQ, node->sobj); break;
        case '!':
          writeChunk(BINARY_NE, node->sobj); break;
        case '>':
          writeChunk(BINARY_GT, node->sobj); break;
        case 'g':
          writeChunk(BINARY_GTEQ, node->sobj); break;
        case '<':
          writeChunk(BINARY_LT, node->sobj); break;
        case 'l':
          writeChunk(BINARY_LTEQ, node->sobj); break;
        case '&':
          writeChunk(BINARY_AND, node->sobj); break;
        case '|':
          writeChunk(BINARY_OR, node->sobj); break;
        

        default:
          internal_error("Unknown binopcode %c", node->as.binop_expr.type);
      }
      break;
    }

    case UnaryExpr:
      exprBytecode(node->as.unary_expr.expr);
      switch(node->as.unary_expr.type) {
        case '-':
          writeChunk(UNARY_MINUS, node->sobj);
          break;
        case '!':
          writeChunk(UNARY_NOT, node->sobj);
          break;

        default:
          internal_error("Unknown unop %c", node->as.unary_expr.type);
      }
      break;
  }
}

void generateBytecode(struct StmtNode* node) {
  switch(node->type) {
    case ExprStmt:
      writeChunk(OP_STORESTACKBASE, node->sobj);
      exprBytecode(node->as.expr_stmt.expr);
      writeChunk(OP_POPTOSTACKBASE, node->sobj);
      break;
    
    case StmtList:
      for(int i = 0; i < node->as.stmt_list.len; i ++) {
        generateBytecode(&node->as.stmt_list.stmts[i]);
      }
      break;

    case IfStmt: {
      exprBytecode(node->as.if_stmt.cond);
      writeChunk(COND_JUMP, node->sobj);
      writeLong(0, node->sobj);
      int pos = currentChunk()->len - 1;
      generateBytecode(node->as.if_stmt.stmts);
      int diff = currentChunk()->len - pos;
      currentChunk()->code[pos - 1] = FIRST_BYTE(diff);
      currentChunk()->code[pos] = SECOND_BYTE(diff);
      break;
    }

    case IfElseStmt: {
      exprBytecode(node->as.ifelse_stmt.cond);
      writeChunk(COND_JUMP, node->sobj);
      writeLong(0, node->sobj);
      int pos = currentChunk()->len - 1;
      generateBytecode(node->as.ifelse_stmt.stmts);
      writeChunk(JUMP, node->sobj);
      writeLong(0, node->sobj);
      int diff = currentChunk()->len - pos;
      currentChunk()->code[pos - 1] = FIRST_BYTE(diff);
      currentChunk()->code[pos] = SECOND_BYTE(diff);

      pos = currentChunk()->len - 1;
      generateBytecode(node->as.ifelse_stmt.elsestmts);
      diff = currentChunk()->len - pos;
      currentChunk()->code[pos - 1] = FIRST_BYTE(diff);
      currentChunk()->code[pos] = SECOND_BYTE(diff);
      break;
    }

    case WhileStmt: {
      int condpos = currentChunk()->len - 1;
      exprBytecode(node->as.if_stmt.cond);
      writeChunk(COND_JUMP, node->sobj);
      writeLong(0, node->sobj);
      int pos = currentChunk()->len - 1;
      generateBytecode(node->as.if_stmt.stmts);
      writeChunk(JUMP_BACK, node->sobj);
      writeLong(currentChunk()->len - condpos, node->sobj);
      int diff = currentChunk()->len - pos;
      currentChunk()->code[pos - 1] = FIRST_BYTE(diff);
      currentChunk()->code[pos] = SECOND_BYTE(diff);
      break;
    }

    case FnDecl: {
      writeChunk(DEF_LBL, node->sobj);
      char* str = node->as.fn_decl.name;
      writeConstant(STRING_VALUE(str));
      writeLong(currentChunk()->poollen - 1, node->sobj);

      // lazy
      writeChunk((uint8_t)node->as.fn_decl.is_lazy, node->sobj);

      writeChunk(JUMP, node->sobj);
      writeLong(0, node->sobj);
      int pos = currentChunk()->len - 1;

      generateBytecode(node->as.fn_decl.stmts);
      writeChunk(OP_RET, node->sobj);
      
      int diff = currentChunk()->len - pos;
      currentChunk()->code[pos - 1] = FIRST_BYTE(diff);
      currentChunk()->code[pos] = SECOND_BYTE(diff);
      break;
    }

    case MyArg: {
      for(int seminum = 0; seminum < node->as.my_arg.num; seminum ++){
        char* str = node->as.my_arg.name[seminum];
  
        writeConstant(STRING_VALUE(str));
        writeChunk(OP_MYARG, node->sobj);
        writeLong(currentChunk()->poollen - 1, node->sobj);
      }
      break;
    }
  }
}

#undef FIRST_BYTE
#undef SECOND_BYTE