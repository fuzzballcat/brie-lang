#ifndef compiler_h
#define compiler_h

#include <stdint.h>

#include "ast.h"
#include "value.h"
#include "error.h"

struct Chunk {
  uint8_t* code;
  int len;
  int capacity;

  struct sObj* lines; // feels wastey

  struct Value* pool;
  int poollen;
  int poolcap;
};

struct Chunk mainChunk;

void generateBytecode(struct StmtNode* stmt);
void initChunk(struct Chunk* chunk);

#endif