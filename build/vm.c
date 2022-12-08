#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "vm.h"
#include "bytecode.h"
#include "debug.h"
#include "error.h"

#define MAX_CALL_STACK 1000

char *inputString(FILE* fp, size_t size){
  char *str = malloc(size * sizeof(char));
  char ch;
  size_t len = 0;
  if(!str) return str;
  while(EOF != (ch=fgetc(fp)) && ch != '\n'){
    str[len++] = ch;
    if(len == size){
      size *= 2;
      str = realloc(str, size * sizeof(char));
      if(!str) return str;
    }
  }
  str[len++] = '\0';

  return realloc(str, sizeof(char)*len);
}

hashtab varmap;

void vm_loadchunk(struct Chunk* chunk) {
  vm.chunk = chunk;
}

void vm_cleanup() {
  free(vm.stack);
  free(vm.callstack);
  free(vm.varstack);
  free(vm.yieldvals);
}

void push(struct Value x) {
  vm.stack[vm.stack_top] = x;
  vm.stack_top ++;
  if (vm.stack_top >= vm.stack_size) {
    vm.stack_size *= 2;
    vm.stack = (struct Value*) realloc(vm.stack, vm.stack_size * sizeof(struct Value));
    if(vm.stack == NULL) {
      runtimeError(-1, "StackError", "Unable to allocate memory for value stack!");
    }
  }
}

struct Value pop() {
  vm.stack_top --;
  if(vm.stack_top < 0) runtimeError(-1, "InternalError", "Stack fault!");
  return vm.stack[vm.stack_top];
}

void execute() {
  vm.stack = (struct Value*) malloc(1 * sizeof(struct Value));
  vm.stack_size = 1;
  vm.stack_top = 0;
  vm.stack_base = 0;

  vm.callstack = (int*) malloc(4 * sizeof(int));
  vm.callstack_size = 4;
  vm.callstack_top = 0;

  vm.varstack = (hashtab**) malloc(1 * sizeof(hashtab*));
  vm.varstack_size = 1;
  vm.varstack_top = 0;

  vm.arg_num = 0;
  
  vm.yieldvals = (struct YieldValsObj*) malloc(1 * sizeof(struct YieldValsObj));
  vm.yieldvals_size = 1;
  vm.yieldvals_top = 0;

  vm.stackbasestack = (int*) malloc(4 * sizeof(int));
  vm.stackbasestack_size = 4;
  vm.stackbasestack_top = 0;
  
  for(int i = 0; i < vm.chunk->len; i ++) {
    enum OpType opcode = (enum OpType) vm.chunk->code[i];
    int line = vm.chunk->lines[i];

#ifdef TRACE
    for(int k = 0; k < vm.stack_top; k ++) {
      if(k == vm.stack_base) printf("\x1b[33m[ \x1b[0m");
      else printf("[ ");
      printValue(vm.stack[k]);
      printf(" ] ");
    }
    if(vm.stack_base == vm.stack_top) printf("\x1b[33m[ \x1b[0m");
    printf("\n\n");
    print_opcode(vm.chunk, i);
#endif

#define PARSE_LONG(ind) (int)(((unsigned)vm.chunk->code[ind + 1] << 8) | vm.chunk->code[ind] )

    switch(opcode) {
      case OP_NONE:
        push(NONE_VALUE());
        break;
      case OP_POP:
        pop();
        break;

      case OP_STORESTACKBASE:
        if(vm.stackbasestack_top >= vm.stackbasestack_size){
          vm.stackbasestack_size *= 2;
          vm.stackbasestack = realloc(vm.stackbasestack, vm.stackbasestack_size * sizeof(int));
        }
        vm.stackbasestack[vm.stackbasestack_top] = vm.stack_top;
        vm.stackbasestack_top ++;
        break;

      case OP_POPTOSTACKBASE:
        vm.stack_top = vm.stackbasestack[vm.stackbasestack_top - 1];
        vm.stackbasestack_top --;
        if(vm.stackbasestack_top < 0) {
          runtimeError(-1, "StackError", "Stackbasestack has reached negative size!");
        }
        break;

      case OP_MYARG: {
        i += 2;
        hash_insert(varmap, vm.chunk->pool[PARSE_LONG(i - 1)].as.string.str, vm.stack[vm.stack_base - vm.arg_num]);
        //if(vm.arg_num >= vm.callstack[vm.callstack_top - 1]) {
        // yield update... this feels unsafe, but it works?
        if(vm.arg_num > vm.stack_base){
          runtimeError(line, "ArgumentError", "Attempt to grab argument with name \"%s\", but no such argument is left!", vm.chunk->pool[PARSE_LONG(i - 1)].as.string.str);
        }
        vm.arg_num ++;
        break;
      }

      case OP_RET: {
        int old_arg_num = vm.arg_num;
        vm.callstack_top -= 2; // call_arity
        vm.arg_num = vm.callstack[vm.callstack_top];
        vm.callstack_top --;
        vm.stack_base = vm.callstack[vm.callstack_top];
        vm.callstack_top --;

        vm.varstack_top --;
        for(int i = 0; i < HASHSIZE; i ++) {
          varmap[i] = (*vm.varstack[vm.varstack_top])[i];
        }
        free(vm.varstack[vm.varstack_top]);

        for(int i = 0; i < old_arg_num; i ++) {
          pop();
        }

        i = vm.callstack[vm.callstack_top];
        if(vm.yieldvals_top == 0) push(NONE_VALUE());
        else {
          struct YieldValsObj yields = 
vm.yieldvals[vm.yieldvals_top - 1];
          for(int yi = yields.top - 1; yi >= 0; yi --){
            push(yields.vals[yi]);
          }

          free(vm.yieldvals[vm.yieldvals_top - 1].vals);
          vm.yieldvals_top --;
          if(vm.yieldvals_top < 0){
            printf("oh no oh no yield stack borkey\n"); 
            exit(1);
          }
        }
        
        break;
      }

      case OP_CALL: {
        struct Value fn = pop();
        struct Value val = pop();
        i += 3;
        double call_arity = vm.chunk->pool[PARSE_LONG(i - 1)].as.num.num;

        if (vm.chunk->code[i - 2] && fn.type == PARAPP){ // force
          // note: force on regular function = no effect
          
          push(val);
          val = fn.as.parapp.args[0];
          
          for(int z = fn.as.parapp.args_len - 1; z >= 1; z --){
            push(fn.as.parapp.args[z]);
          }

          // hacky hacky to copy logic
          fn = (struct Value){
            .type = FUN,
            .as.fun = {
              .name = fn.as.parapp.fn.name,
              .label = fn.as.parapp.fn.label,
              .is_lazy = 0
            }
          };
        }
        
        if(fn.type == PARAPP){
          fn.as.parapp.args = realloc(fn.as.parapp.args, (fn.as.parapp.args_len + call_arity) * sizeof(struct Value));
          fn.as.parapp.args[fn.as.parapp.args_len] = val;
          for(int z = fn.as.parapp.args_len + 1; z < fn.as.parapp.args_len + call_arity; z ++){
            fn.as.parapp.args[z] = pop();
          }
          
          fn.as.parapp.args_len += call_arity;

          push(fn);
        } else if(fn.type == FUN && fn.as.fun.is_lazy){
          struct Value* args = malloc(call_arity * sizeof(struct Value));
          args[0] = val;
          for(int z = 1; z < call_arity; z ++){
            args[z] = pop();
          }

          push(PARAPP_VALUE(fn.as.fun.name, fn.as.fun.label, fn.as.fun.is_lazy,args,call_arity));
        } else if (fn.type == FUN) {
          vm.callstack[vm.callstack_top] = i;
          vm.callstack_top ++;

          vm.callstack[vm.callstack_top] = vm.stack_base;
          vm.callstack_top ++;

          vm.callstack[vm.callstack_top] = vm.arg_num;
          vm.callstack_top ++;

          vm.callstack[vm.callstack_top] = call_arity;
          vm.callstack_top ++;

          vm.stack_base = vm.stack_top;
          vm.arg_num = 0;

          vm.varstack[vm.varstack_top] = calloc(1, sizeof(hashtab));
          hashtab* newhash = clone_hash(varmap);
          for(int i = 0; i < HASHSIZE; i ++) {
            (*vm.varstack[vm.varstack_top])[i] = (*newhash)[i];
          }

          vm.varstack_top ++;

          if(vm.varstack_top >= vm.varstack_size) {
            vm.varstack_size *= 2;
            vm.varstack = (hashtab**) realloc(vm.varstack, vm.varstack_size * sizeof(hashtab*));
          }

          if(vm.callstack_top >= vm.callstack_size) {
            vm.callstack_size *= 2;
            vm.callstack = (int*) realloc(vm.callstack, vm.callstack_size * sizeof(int));
          }

          if (vm.callstack_top >= MAX_CALL_STACK) {
            runtimeError(line, "StackError", "Max recursion depth reached!");
          }

          if(vm.callstack == NULL) {
            runtimeError(-1, "StackError", "Unable to allocate memory for call stack!");
          }

          if(vm.yieldvals_top >= vm.yieldvals_size){
            vm.yieldvals_size *= 2;
            vm.yieldvals = realloc(vm.yieldvals, vm.yieldvals_size * sizeof(struct YieldValsObj));
          }
          vm.yieldvals[vm.yieldvals_top] = (struct YieldValsObj) {
            .size = 1,
            .top = 0,
            .vals = (struct Value*)malloc(1 * sizeof(struct Value))
          };
          vm.yieldvals_top ++;

          push(val);

          i = fn.as.fun.label + 4;
        } else if(fn.type == NATIVEFN) {
          if(strcmp(fn.as.nativefn.name, "print") == 0) {
            if(call_arity != 1) runtimeError(line, "ArgumentError", "Native function \"print\" expects only one argument but received %d", call_arity);
            printValue(val);
            putchar('\n');
            push(NONE_VALUE());
          } else if(strcmp(fn.as.nativefn.name, "int") == 0) {
            if(call_arity != 1) runtimeError(line, "ArgumentError", "Native function \"int\" expects only one argument but received %d", call_arity);
            push(toIntVal(line, val));
          } else if(strcmp(fn.as.nativefn.name, "str") == 0) {
            if(call_arity != 1) runtimeError(line, "ArgumentError", "Native function \"str\" expects only one argument but received %d", call_arity);
            push(toStrVal(line, val));
          } else if(strcmp(fn.as.nativefn.name, "rc") == 0){
            if(call_arity != 1 || val.type != NUM) runtimeError(line, "ArgumentError", "Native function \"rc\" expects only one argument of type NUM, but received %d argument(s) of type %s", call_arity, typeOf(val));

          if((int)val.as.num.num > vm.stack_base || (int)val.as.num.num < 0){
            runtimeError(line, "ArgumentError", "Attempt to recall argument %d but no such argument exists!", (int)val.as.num.num);
          }
          push(vm.stack[vm.stack_base - (int)val.as.num.num]);
          } else if(strcmp(fn.as.nativefn.name, "return") == 0) {
            if(vm.yieldvals_top == 0){
              runtimeError(line, "ReturnError", "Cannot return outside of a function!");
            }
            int old_arg_num = vm.arg_num;
            vm.callstack_top -= 2; // call_arity
            vm.arg_num = vm.callstack[vm.callstack_top];
            vm.callstack_top --;
            vm.stack_base = vm.callstack[vm.callstack_top];
            vm.callstack_top --;

            vm.varstack_top --;
            for(int i = 0; i < HASHSIZE; i ++) {
              varmap[i] = (*vm.varstack[vm.varstack_top])[i];
            }
            free(vm.varstack[vm.varstack_top]);

            for(int i = 0; i < old_arg_num; i ++) {
              pop();
            }

            i = vm.callstack[vm.callstack_top];
            push(val);

            // yield stuffs
            struct YieldValsObj yields = vm.yieldvals[vm.yieldvals_top - 1];
            for(int yi = yields.top - 1; yi >= 0; yi --){
              push(yields.vals[yi]);
            }

            free(vm.yieldvals[vm.yieldvals_top - 1].vals);
            vm.yieldvals_top --;
            if(vm.yieldvals_top < 0){
              printf("oh no oh no yield stack borkey\n"); 
              exit(1);
            }

            // undo the "push stack base" that came before, return is technically an expression
            vm.stackbasestack_top --;
            if(vm.stackbasestack_top < 0) {
              runtimeError(-1, "StackError", "Stackbasestack has reached negative size!");
            }

            break;
          } else if(strcmp(fn.as.nativefn.name, "yields") == 0){
            if(vm.yieldvals_top == 0){
              runtimeError(line, "YieldError", "Cannot yield outside of a function!");
            }
            vm.yieldvals[vm.yieldvals_top - 1].vals[vm.yieldvals[vm.yieldvals_top - 1].top] = val;
            vm.yieldvals[vm.yieldvals_top - 1].top ++;
            if(vm.yieldvals[vm.yieldvals_top - 1].top >= vm.yieldvals[vm.yieldvals_top - 1].size){
              vm.yieldvals[vm.yieldvals_top - 1].size *= 2;
              vm.yieldvals[vm.yieldvals_top - 1].vals = (struct Value*) realloc(vm.yieldvals[vm.yieldvals_top - 1].vals, vm.yieldvals[vm.yieldvals_top - 1].size * sizeof(struct Value));
            }
            
            push(val); // i guess you could want this?  i dunno
          } else if(strcmp(fn.as.nativefn.name, "input") == 0) {
            switch(val.type) {
              case NONE: break;
              case STRING: printValue(val); break;

              default:
                runtimeError(line, "TypeError", "Expect argument of type str or None to function \"input\", received argument of type %s!", typeOf(val));
            }
            push(STRING_VALUE(inputString(stdin, 10)));
          } else if(strcmp(fn.as.nativefn.name, "slice") == 0){
            if(call_arity != 3){
              runtimeError(line, "ArgumentError", "Native function \"slice\" requires three arguments but recieved %d", call_arity);
            }
            if(val.type != STRING){
              runtimeError(line, "TypeError", "Expect first argument to native function \"slice\" to be STRING, received argument of type %s!", typeOf(val));
            }
            struct Value i0 = pop(), i1 = pop();
            if(i0.type != NUM || i1.type != NUM){
              runtimeError(line, "TypeError", "Indices to native function \"slice\" must be integers, received argument of type %s!", typeOf(val));
            }
            size_t start = (size_t) i0.as.num.num,
                    end  = (size_t) i1.as.num.num,
                    len  = end - start + 1; // nullchar
            char* newstrbuf = (char*) malloc(len * sizeof(char));
            strncpy(newstrbuf, val.as.string.str + start, len - 1);
            free(val.as.string.str);
            newstrbuf[len - 1] = '\0';
            val.as.string.str = newstrbuf;
            push(val);
          } else if(strcmp(fn.as.nativefn.name, "len") == 0){
            if(call_arity != 1 || val.type != STRING){
              runtimeError(line, "ArgumentError", "Native fucntion \"len\" expects one argument of type STRING, recieved %d argument(s) of type %s!", call_arity, typeOf(val));
            }

            push((struct Value){ .type = NUM, .as.num.num = strlen(val.as.string.str) });
          } else {
            runtimeError(line, "InternalError", "Unknown native function \"%s\"", fn.as.nativefn.name);
          }
        } else {
          runtimeError(line, "TypeError", "Attempting to call non-callable!");
        }

        break;
      }

      case OP_ASSIGN: {
        struct Value val = pop();
        char* str = vm.chunk->pool[PARSE_LONG(i + 1)].as.string.str;
        hash_insert(varmap, str, val);
        //push(val);
        i += 2;
        break;
      }

      case OP_CONSTANT: {
        struct Value val;
        copyVal(&val, &vm.chunk->pool[PARSE_LONG(i + 1)]);
        push(val);
        i += 2;
        break;
      }

      case OP_LOAD: {
        char* str = vm.chunk->pool[PARSE_LONG(i + 1)].as.string.str;
        struct hash_list_node* lookup = hash_lookup(varmap, str);
        if(lookup == NULL) runtimeError(line, "NameError", "Name '%s' is not defined", str);
        struct Value val;
        copyVal(&val, &lookup->value);
        push(val);
        i += 2;
        break;
      }

      case UNARY_MINUS: {
        struct Value val = pop();
        if(val.type != NUM) runtimeError(line, "TypeError", "Unary - is not defined for values of type %s!", typeOf(val));
        val.as.num.num = -val.as.num.num;
        push(val);
        break;
      }
      case UNARY_NOT: {
        struct Value val = pop();
        if(val.type == NONE) {
          push(NUM_VALUE(1));
        } else if (val.type == NUM) {
          if(val.as.num.num == 0) push(NUM_VALUE(1));
          else push(NUM_VALUE(0));
        } else {
          push(NUM_VALUE(0));
        }
        break;
      }


      case BINARY_ADD: 
      case BINARY_SUB:
      case BINARY_MUL:
      case BINARY_DIV:
      case BINARY_EQEQ:
      case BINARY_NE:
      case BINARY_LT:
      case BINARY_GT:
      case BINARY_LTEQ: 
      case BINARY_GTEQ:
      case BINARY_AND:
      case BINARY_OR: {
        struct Value left = pop();
        struct Value right = pop();
        switch(left.type) {
          case STRING: {
            if(right.type != STRING) runtimeError(line, "TypeError", "Cannot apply operator %s to values of type %s and %s!", binopToStr(opcode), typeOf(left), typeOf(right));
            if(opcode != BINARY_ADD && opcode != BINARY_EQEQ && opcode != BINARY_NE) runtimeError(line, "TypeError", "Operator %s is not defined values of type string and string!", binopToStr(opcode));

            if(opcode == BINARY_ADD) {
              int leftlen = strlen(left.as.string.str);
              int rightlen = strlen(right.as.string.str);

              left.as.string.str = (char*)realloc(left.as.string.str, (leftlen + rightlen + 1) * sizeof(char));

              memcpy(left.as.string.str + leftlen, right.as.string.str, rightlen + 1);

              push(left);
              freeValue(right);
            } else if (opcode == BINARY_EQEQ) {
              push(NUM_VALUE(strcmp(left.as.string.str, right.as.string.str) == 0));
              freeValue(left);
              freeValue(right);
            } else if (opcode == BINARY_NE) {
              push(NUM_VALUE(strcmp(left.as.string.str, right.as.string.str) != 0));
              freeValue(left);
              freeValue(right);
            }
            break;
          }
          case NUM:
            if(right.type != NUM) runtimeError(line, "TypeError", "Cannot apply binary op %s to values of type %s and %s!", binopToStr(opcode), typeOf(left), typeOf(right));
            double value = left.as.num.num;
            switch(opcode) {
              case BINARY_ADD:
                value += right.as.num.num;
                break;
              case BINARY_SUB:
                value -= right.as.num.num;
                break;
              case BINARY_MUL:
                value *= right.as.num.num;
                break;
              case BINARY_DIV:
                value /= right.as.num.num;
                break;
              case BINARY_EQEQ:
                value = value == right.as.num.num;
                break;
              case BINARY_NE:
                value = value != right.as.num.num;
                break;
              case BINARY_GT:
                value = value > right.as.num.num;
                break;
              case BINARY_GTEQ:
                value = value >= right.as.num.num;
                break;
              case BINARY_LT:
                value = value < right.as.num.num;
                break;
              case BINARY_LTEQ:
                value = value <= right.as.num.num;
                break;
              case BINARY_AND:
                value = value && right.as.num.num;
                break;
              case BINARY_OR:
                value = value || right.as.num.num;
                break;

              default: runtimeError(line, "InternalError", "UnknownBinaryOp");
            }

            push(NUM_VALUE(value));
            break;

          default:
            runtimeError(line, "TypeError", "Cannot apply binary op %s to values of type %s and %s!", binopToStr(opcode), typeOf(left), typeOf(right));
        }

        break;
      }

      case COND_JUMP: {
        struct Value val = pop();
        if(val.as.num.num == 0) {
          i += PARSE_LONG(i + 1) + 1;
        } else i += 2;
        break;
      }
      
      case JUMP: {
        i += PARSE_LONG(i + 1) + 1;
        break;
      }

      case JUMP_BACK: {
        i -= PARSE_LONG(i + 1) - 1;
        break;
      }

      case DEF_LBL: {
        hash_insert(varmap, vm.chunk->pool[PARSE_LONG(i + 1)].as.string.str, FUN_VALUE(vm.chunk->pool[PARSE_LONG(i + 1)].as.string.str, i + 2, vm.chunk->code[i + 3]));
        i += 3;
        break;
      }
    }
  }

#ifdef TRACE
  for(int k = 0; k < vm.stack_top; k ++) {
    if(k == vm.stack_base) printf("\x1b[33m[ \x1b[0m");
    else printf("[ ");
    printValue(vm.stack[k]);
    printf(" ] ");
  }
  if(vm.stack_base == vm.stack_top) printf("\x1b[33m[ \x1b[0m");
  printf("\n");
#endif
}