#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "value.h"
#include "error.h"

char* typeOf(struct Value val) {
#define MAKE_STR(len,x) do { char* buff = malloc(len * sizeof(char)); sprintf(buff, x); return buff; } while(0)
  switch(val.type) {
    case STRING: MAKE_STR(4, "str");
    case NATIVEFN: MAKE_STR(12, "<native fn>");
    case NUM: MAKE_STR(4, "int");
    case FUN: MAKE_STR(5, "<fn>");
    case NONE: MAKE_STR(5, "None");
    case PARAPP: MAKE_STR(12, "<parapp fn>");
  }
#undef MAKE_STR
}

void printValue(struct sObj sobj, struct Value val) {
  struct Value string = toStrVal(sobj, val);

  printf("%s", string.as.string.str);
}

void freeValue(struct Value val) {
  switch(val.type) {
    case NONE:
    case NUM: break;

    case STRING:
      free(val.as.string.str);
      break;
    case NATIVEFN:
      free(val.as.nativefn.name);
      break;
    case FUN:
      free(val.as.fun.name);
      break;

    case PARAPP:
      free(val.as.parapp.fn.name);
      for(int i = 0; i < val.as.parapp.args_len; i ++){
        freeValue(val.as.parapp.args[i]);
      }
      free(val.as.parapp.args);
      break;
  }
}

void copyVal(struct Value* dest, struct Value* src) {
  switch(src->type) {
    case NONE:
    case NUM:
      memcpy(dest, src, sizeof(struct Value));
      break;

    case STRING: {
      memcpy(dest, src, sizeof(struct Value));
      int len = strlen(src->as.string.str) + 1;
      char* newstr = (char*)malloc(len);
      memcpy(newstr, src->as.string.str, len);
      dest->as.string.str = newstr;
      break;
    }

    case NATIVEFN: {
      memcpy(dest, src, sizeof(struct Value));
      int len = strlen(src->as.nativefn.name) + 1;
      char* newstr = (char*)malloc(len);
      memcpy(newstr, src->as.nativefn.name, len);
      dest->as.nativefn.name = newstr;
      break;
    }

    case FUN: {
      memcpy(dest, src, sizeof(struct Value));
      int len = strlen(src->as.fun.name) + 1;
      char* newstr = (char*)malloc(len);
      memcpy(newstr, src->as.fun.name, len);
      dest->as.fun.name = newstr;
      break;
    }

    case PARAPP: {
      memcpy(dest, src, sizeof(struct Value));
      
      int len = strlen(src->as.parapp.fn.name) + 1;
      char* newstr = (char*)malloc(len);
      memcpy(newstr, src->as.parapp.fn.name, len);
      dest->as.parapp.fn.name = newstr;

      for(int i = 0; i < dest->as.parapp.args_len; i ++){
        copyVal(&dest->as.parapp.args[i], &dest->as.parapp.args[i]);
      }
      break;
    }
  }
}

struct Value toIntVal(struct sObj sobj, struct Value value) {
  switch(value.type){
    case NUM:
      return value;
    
    case STRING: {
      double d;
      // validate
      char* str = value.as.string.str;
      if (*str == '-')
        ++str;

      if (!*str) general_error(sobj, "TypeError", "The string must be a sequence of digits optionally preceded by a single negative sign.", "Invalid candidate for integer conversion \"%s\"!", value.as.string.str);

      while (*str)
      {
        if (!isdigit(*str))
          general_error(sobj, "TypeError", "The string must be a sequence of digits optionally preceded by a single negative sign.", "Invalid candidate for integer conversion \"%s\"!", value.as.string.str);
        else
          ++str;
      }
      // end
      sscanf(value.as.string.str, "%lf", &d);
      return NUM_VALUE(d);
    }

    case NATIVEFN:
      general_error(sobj, "TypeError", "A function cannot be converted to an integer.", "Invalid candidate for integer conversion <native fn %s>!", value.as.nativefn.name);

    case FUN:
      general_error(sobj, "TypeError", "A function cannot be converted to an integer.", "Invalid candidate for integer conversion <fn %s>!", value.as.fun.name);

    case PARAPP:
      general_error(sobj, "TypeError", "A function cannot be converted to an integer.", "Invalid candidate for integer conversion <partial fn %s>!", value.as.parapp.fn.name);
    
    case NONE:
      general_error(sobj, "TypeError", "None cannot be converted to an integer (perhaps add a None check).", "Invalid candidate for integer conversion None!");
  }
}

struct Value toStrVal(struct sObj sobj, struct Value value){
  switch(value.type) {
    case STRING:
      return value;
    
    case NUM: {
      size_t needed = snprintf(NULL, 0, "%g", value.as.num.num) + 1;
      char* buffer = (char*)malloc(needed * sizeof(char));
      sprintf(buffer, "%g", value.as.num.num);
      return STRING_VALUE(buffer);
    }

    case NATIVEFN: {
      size_t needed = snprintf(NULL, 0, "<native fn %s>", value.as.nativefn.name) + 1;
      char* buffer = (char*)malloc(needed * sizeof(char));
      sprintf(buffer, "<native fn %s>", value.as.nativefn.name);
      return STRING_VALUE(buffer);
    }

    case FUN: {
      size_t needed = snprintf(NULL, 0, "<fn %s>", value.as.fun.name) + 1;
      char* buffer = (char*)malloc(needed * sizeof(char));
      sprintf(buffer, "<fn %s>", value.as.fun.name);
      return STRING_VALUE(buffer);
    }

    case PARAPP: {
      size_t needed = snprintf(NULL, 0, "<partial fn %s>", value.as.parapp.fn.name);
      char* buffer = (char*)malloc(needed * sizeof(char));
      sprintf(buffer, "<partial fn %s>", value.as.parapp.fn.name);
      return STRING_VALUE(buffer);
    }

    case NONE: {
      char* buffer = (char*)malloc(5 * sizeof(char));
      sprintf(buffer, "None");
      return STRING_VALUE(buffer);
    }
  }
}