#ifndef error_h
#define error_h

#include <stddef.h>

struct sObj {
  size_t line, display_line, col, len;
  char *source_name;
};

void general_error(struct sObj sdetail, char* type, char* hint, char* fmtstr, ...) __attribute((noreturn));

void file_only_error(char* filename, char* type, char* hint, char* fmtstr, ...) __attribute__((noreturn));

void internal_error(char* fmtstr, ...) __attribute__((noreturn));

#endif