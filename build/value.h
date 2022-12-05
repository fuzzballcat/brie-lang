#ifndef value_h
#define value_h

struct Value {
  enum {
    STRING,
    NATIVEFN,
    NUM,
    FUN,
    NONE
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
    } fun;
  } as;
};

char* typeOf(struct Value);
void printValue(struct Value);
void freeValue(struct Value);
void copyVal(struct Value*, struct Value*);

struct Value toIntVal(int line, struct Value);
struct Value toStrVal(int line, struct Value);

#define STRING_VALUE(x) (struct Value){ .type = STRING, .as.string.str = x }
#define NATIVEFN_VALUE(nm) (struct Value){ .type = NATIVEFN, .as.nativefn.name = nm }
#define NUM_VALUE(nm) (struct Value){ .type = NUM, .as.num.num = nm }
#define FUN_VALUE(nm,lbl) (struct Value){ .type = FUN, .as.fun = { .name = nm, .label = lbl } }
#define NONE_VALUE() (struct Value){ .type = NONE }

#endif