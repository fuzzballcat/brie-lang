#ifndef vm_h
#define vm_h

#include "compiler.h"
#include "hash.h"

struct YieldValsObj {
  struct Value* vals;
  int size;
  int top;
};

struct {
  struct Chunk* chunk;

  struct Value* stack;
  int stack_size;
  int stack_top;
  int stack_base;

  int* callstack;
  int callstack_size;
  int callstack_top;

  hashtab** varstack;
  int varstack_size;
  int varstack_top;

  int arg_num;

  struct YieldValsObj* yieldvals;
  int yieldvals_size;
  int yieldvals_top;

  int* stackbasestack; // hmst
  int stackbasestack_size;
  int stackbasestack_top;
} vm;

void execute();
void vm_loadchunk(struct Chunk* chunk);
void vm_cleanup();

#endif