#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "scanner.h"

void general_error(struct sObj sdetail, char* type, char* hint, char* fmtstr, ...){
  va_list args;
  va_start(args, fmtstr);
  
  printf("\x1b[36m[%s] \x1b[31m%s:\x1b[0m ", sdetail.source_name, type);
  vprintf(fmtstr, args);

  printf("\n\n");
  printf("\x1b[35m%zu |\x1b[0m", sdetail.display_line);
  size_t fmtlen = snprintf(NULL, 0, "%zu |", sdetail.display_line);
  
  size_t srclen = strlen(scanner.source),
         nls    = 1;
  for(size_t i = 0; i < srclen; i ++){
    char ch = scanner.source[i];
    if(nls == sdetail.line){
      putchar(ch);
    }
    
    if(ch == '\n') nls ++;
  }
  for(size_t i = 0; i < sdetail.col + fmtlen - 1; i ++){
    putchar(' ');
  }
  printf("\x1b[34m");
  for(size_t i = 0; i < sdetail.len; i ++){
    putchar('^');
  }

  if(hint != NULL) printf("\n\n\x1b[33mHint: %s", hint);
  printf("\x1b[0m\n");
  
  va_end(args);
  
  exit(1);
}

void file_only_error(char* filename, char* type, char* hint, char* fmtstr, ...) {
  va_list args;
  va_start(args, fmtstr);
  
  printf("\x1b[36m[%s] \x1b[31m%s:\x1b[0m ", filename, type);
  vprintf(fmtstr, args);
  if(hint != NULL) printf("\n\n\x1b[33mHint: %s\x1b[0m\n", hint);
  
  va_end(args);
  
  exit(1);
}

void internal_error(char* fmtstr, ...){
  va_list args;
  va_start(args, fmtstr);
  printf("\x1b[31mInternalError:\x1b[0m ");
  vprintf(fmtstr, args);
  printf("\n\n\x1b[33mHint: If you're seeing this, something has gone horribly wrong internally.\x1b[0m\n");
  va_end(args);

  exit(1);
}