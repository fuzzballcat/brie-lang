#ifndef bytecode_h
#define bytecode_h

#include <stdint.h>

#include "compiler.h"

enum OpType {
  OP_CALL,
  OP_CONSTANT,
  OP_ASSIGN,
  OP_LOAD,
  BINARY_ADD,
  BINARY_SUB,
  BINARY_MUL,
  BINARY_DIV,
  BINARY_EQEQ,
  BINARY_NE,
  BINARY_LT,
  BINARY_LTEQ,
  BINARY_GT,
  BINARY_GTEQ,
  COND_JUMP,
  JUMP,
  JUMP_BACK,
  DEF_LBL,
  OP_RET,
  OP_MYARG,
  OP_POP,
  OP_STORESTACKBASE,
  OP_POPTOSTACKBASE,
  OP_NONE,
  UNARY_MINUS,
  UNARY_NOT,
  BINARY_AND,
  BINARY_OR
};

char* binopToStr(enum OpType);
void print_bytecode(struct Chunk*);
int print_opcode(struct Chunk*, int);

#endif