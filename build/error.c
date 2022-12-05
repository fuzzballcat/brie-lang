#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "error.h"

void runtimeError(int line, char* type, char* format, ...) {
  va_list args;
  va_start(args, format);
  printf("\x1b[33m[line %d] \x1b[31m%s:\x1b[0m ", line, type);
  vprintf(format, args);
  va_end(args);
  printf("\n");

  exit(1);
}