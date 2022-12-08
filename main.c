#include <stdio.h>
#include <stdlib.h>

#include "./build/debug.h"

#include "./build/scanner.h"
#include "./build/parser.h"
#include "./build/ast.h"
#include "./build/compiler.h"
#include "./build/bytecode.h"
#include "./build/vm.h"

char *read_file(char *fname) {
  FILE *f = fopen(fname, "r");
  char *buffer = NULL;
  int string_size, read_size;

  if(f) {
    fseek(f, 0, SEEK_END);
    string_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = malloc(sizeof(char) * (string_size + 2));
    buffer[string_size] = '\n';
    buffer[string_size + 1] = '\0';

    read_size = fread(buffer, sizeof(char), string_size, f);

    if(string_size != read_size) {
      free(buffer);
      printf("Unable to allocate file memory!\n");
      exit(1);
    }
    
    fclose(f);

    return buffer;
  } else {
    printf("Unable to read file!\n");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  if(argc != 2){
    printf("Expected exactly one argument to the command line.\n");
    exit(1);
  }
  
  char *contents = read_file(argv[1]);

  initScanner(contents);
  initChunk(&mainChunk);
  generateBytecode(parse());
  cleanupScanner();

#ifdef PREPRINT
  print_bytecode(&mainChunk);
#endif
  vm_loadchunk(&mainChunk);
  execute();
  vm_cleanup();

  return 0;
}