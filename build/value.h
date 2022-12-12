#ifndef value_h
#define value_h

#include <stddef.h>

#include "error.h"

struct Value {
  enum {
    STRING,
    NATIVEFN,
    NUM,
    FUN,
    NONE,
    PARAPP
  } type;

  union {
    struct {
      char* str;
    } string;

    struct {
      char* name;
    } nativefn;

    struct {
      double num;
    } num;

    struct {
      char* name;
      int label;
      int is_lazy;
    } fun;

    struct {
      struct {
        char* name;
        int label;
        int is_lazy;
      } fn; // extremely cheaty, but we are avoiding malloc

      struct Value* args;
      int args_len;
    } parapp;
  } as;
};

char* typeOf(struct Value);
void printValue(struct sObj, struct Value);
void freeValue(struct Value);
void copyVal(struct Value*, struct Value*);

struct Value toIntVal(struct sObj, struct Value);
struct Value toStrVal(struct sObj, struct Value);

#define STRING_VALUE(x) (struct Value){ .type = STRING, .as.string.str = x }
#define NATIVEFN_VALUE(nm) (struct Value){ .type = NATIVEFN, .as.nativefn.name = nm }
#define NUM_VALUE(nm) (struct Value){ .type = NUM, .as.num.num = nm }
#define FUN_VALUE(nm,lbl,islzy) (struct Value){ .type = FUN, .as.fun = { .name = nm, .label = lbl, .is_lazy = islzy } }
#define NONE_VALUE() (struct Value){ .type = NONE }
#define PARAPP_VALUE(fnname,fnlbl,fnislazy,ags,agslen) (struct Value){ .type = PARAPP, .as.parapp = { .fn = { .name = fnname, .label = fnlbl, .is_lazy = fnislazy }, .args = ags, .args_len = agslen } }

#endif