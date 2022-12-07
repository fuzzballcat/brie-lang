#include <stdio.h>
#include <stdlib.h>

#include "bytecode.h"

int print_opcode(struct Chunk* chunk, int i) {
#define PARSE_LONG(ind) (int)(((unsigned)chunk->code[ind + 1] << 8) | chunk->code[ind] )

  enum OpType op = (enum OpType) chunk->code[i];
  switch (op) {
    case OP_NONE:
      printf("OP_NONE\n");
      return 1;
    case OP_POP:
      printf("OP_POP\n");
      return 1;

    case OP_POPTOSTACKBASE:
      printf("OP_POPTOSTACKBASE\n");
      return 1;
    case OP_STORESTACKBASE:
      printf("OP_STORESTACKBASE\n");
      return 1;
    
    case OP_CALL:
      printf("OP_CALL:     %g", chunk->pool[PARSE_LONG(i + 2)].as.num.num);
      if(chunk->code[i + 1]) printf(" [force]");
      printf("\n");
      return 4;
    case OP_CONSTANT:
      printf("OP_CONSTANT: ");
      printValue(chunk->pool[PARSE_LONG(i + 1)]);
      putchar('\n');
      return 3;
    case OP_ASSIGN:
      printf("OP_ASSIGN:   %s\n", chunk->pool[PARSE_LONG(i + 1)].as.string.str);
      return 3;
    case OP_LOAD:
      printf("OP_LOAD:     %s\n", chunk->pool[PARSE_LONG(i + 1)].as.string.str);
      return 3;

    case BINARY_ADD:
      printf("BINARY_ADD\n"); return 1;
    case BINARY_SUB:
      printf("BINARY_SUB\n"); return 1;
    case BINARY_MUL:
      printf("BINARY_MUL\n"); return 1;
    case BINARY_DIV:
      printf("BINARY_DIV\n"); return 1;
    case BINARY_EQEQ:
      printf("BINARY_EQEQ\n"); return 1;
    case BINARY_NE:
      printf("BINARY_NE\n"); return 1;
    case BINARY_GT:
      printf("BINARY_GT\n"); return 1;
    case BINARY_GTEQ:
      printf("BINARY_GTEQ\n"); return 1;
    case BINARY_LT:
      printf("BINARY_LT\n"); return 1;
    case BINARY_LTEQ:
      printf("BINARY_LTEQ\n"); return 1;
    case BINARY_AND:
      printf("BINARY_AND\n"); return 1;
    case BINARY_OR:
      printf("BINARY_OR\n"); return 1;
    
    case UNARY_MINUS:
      printf("UNARY_MINUS\n"); return 1;
    case UNARY_NOT:
      printf("UNARY_NOT\n"); return 1;

    case COND_JUMP:
      printf("COND_JUMP:   %d\n", PARSE_LONG(i + 1)); return 3;
    case JUMP:
      printf("JUMP:        %d\n", PARSE_LONG(i + 1)); return 3;
    case JUMP_BACK:
      printf("JUMP_BACK:   %d\n", PARSE_LONG(i + 1)); return 3;
    case DEF_LBL:
      printf("DEF_LBL:     %s (lz: %d)\n", chunk->pool[PARSE_LONG(i + 1)].as.string.str, chunk->code[i + 3]);
      return 4;
    case OP_RET:
      printf("OP_RET\n");
      return 1;
    
    case OP_MYARG:
      printf("OP_MYARG:    %s\n", chunk->pool[PARSE_LONG(i + 1)].as.string.str);
      return 3;
  }
}

void print_bytecode(struct Chunk* chunk) {
  printf("---- pool ----\n");
  for(int i = 0; i < chunk->poollen; i ++) {
    printValue(chunk->pool[i]);
    printf("\n");
  }
  printf("---- code ----\n");
  for(int i = 0; i < chunk->len; i += print_opcode(chunk, i));
}

char* binopToStr(enum OpType op) {
#define MAKE_STR(len,x) do { char* buff = malloc(len * sizeof(char)); sprintf(buff, x); return buff; } while(0)
  switch(op) {
    case BINARY_GT: MAKE_STR(2, ">");
    case BINARY_LT: MAKE_STR(2, "<");
    case BINARY_GTEQ: MAKE_STR(3, ">=");
    case BINARY_LTEQ: MAKE_STR(3, "<=");
    case BINARY_NE: MAKE_STR(3, "!=");
    case BINARY_EQEQ: MAKE_STR(3, "==");
    case BINARY_ADD: MAKE_STR(2, "+");
    case BINARY_SUB: MAKE_STR(2, "-");
    case BINARY_MUL: MAKE_STR(2, "*");
    case BINARY_DIV: MAKE_STR(2, "/");

    default:
      printf("InternalError: Invalid Optype for binop to str!");
      exit(1);
  }
#undef MAKE_STR
}