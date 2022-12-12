#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "./build/debug.h"

#include "./build/scanner.h"
#include "./build/parser.h"
#include "./build/ast.h"
#include "./build/compiler.h"
#include "./build/bytecode.h"
#include "./build/vm.h"
#include "./build/error.h"

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

char* find_module_file(char* path, char* module_name){
  DIR *dir;
  char *slash = "/";

  struct dirent *entry;
  if((dir = opendir(path)) == NULL){
    fprintf(stderr, "Cannot open directory"); 
    exit(1);
  }

  while((entry = readdir(dir))){
    if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
      continue;

    int length = strlen(path) + strlen(entry->d_name)+2;
    char *newpath = malloc(length);
    if (!newpath){
      fprintf(stderr, "malloc fail");
      break;
    }
    snprintf(newpath,length,"%s%s%s",path,slash,entry->d_name);
    
    if(entry->d_type == DT_DIR){
      char* possible_ret = find_module_file(newpath, module_name);
      if(possible_ret != NULL) {
        free(newpath);
        return possible_ret;
      }
    }
    else {
      char *ext = strrchr(entry->d_name,'.');
      if(ext && ext != entry->d_name && strcmp(ext, ".br") == 0){
        char* read_string = read_file(newpath);

        int len = 5 + strlen(module_name);
        char* unit_decl = malloc(len + 1); // whaa
        snprintf(unit_decl,len + 1,"unit %s", module_name);
        
        if(strncmp(read_string, unit_decl, len) == 0){
          // found file
          free(unit_decl);
          free(newpath);
          return read_string;
        }

        free(unit_decl);
      }
    }

    free(newpath);
  }
  if(closedir(dir) != 0){
    fprintf(stderr,"closedir fail");
    exit(1);
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  if(argc != 2){
    printf("ERROR: Expected exactly one argument to the command line.\n");
    exit(1);
  }
  
  char *contents = read_file(argv[1]);
  
  // module workflow
  if(strncmp(contents, "unit", 4) == 0) {
    int RESTORE_NEWLINES = 0;
    char* fstart = contents;
    while(*fstart != ' '){
      fstart++;
    }
    fstart++;
    char* mdnmstart = fstart;
    while(*fstart != '\n') fstart ++;
    fstart ++; RESTORE_NEWLINES ++;
    char* this_module_name = (char*)malloc((fstart - mdnmstart) * sizeof(char));
    strncpy(this_module_name, mdnmstart, fstart - mdnmstart - 1);
    this_module_name[fstart - mdnmstart - 1] = '\0';
    //printf("%s\n", module_name);
    
    while(*fstart == '\n') { fstart ++; RESTORE_NEWLINES ++; }
    
    char** import_queue = (char**)malloc(1 * sizeof(char*));
    int import_queue_size = 1, import_queue_top = 0;

    char** where_imported_from_queue = (char**)malloc(1 * sizeof(char*));
    int where_imported_from_queue_size = 1, where_imported_from_queue_top = 0;
    
    while(strncmp(fstart, "uses ", 5) == 0){
      fstart += 5;
      char* import_name_start = fstart;
      while(*fstart != '\n') fstart ++;

      char* import_name = (char*)malloc((fstart - import_name_start + 1) * sizeof(char));
      strncpy(import_name, import_name_start, fstart - import_name_start);
      import_name[fstart - import_name_start] = '\0';

      int do_add = strcmp(import_name, this_module_name);
      for(int i = 0; i < import_queue_top; i ++){
        if(!strcmp(import_name, import_queue[i])) {
          do_add = 0;
          break;
        }
      }

      if(do_add){
        import_queue[import_queue_top] = import_name;
        import_queue_top++;
        if(import_queue_top >= import_queue_size){
          import_queue = realloc(import_queue, (import_queue_size *= 2) * sizeof(char*));
        }

        where_imported_from_queue[where_imported_from_queue_top] = this_module_name;
        where_imported_from_queue_top++;
        if(where_imported_from_queue_top >= where_imported_from_queue_size){
          where_imported_from_queue = realloc(where_imported_from_queue, (where_imported_from_queue_size *= 2) * sizeof(char*));
        }
      }
      
      while(*fstart == '\n') { fstart++; RESTORE_NEWLINES ++; }
    }

    char* new_contents = (char*)malloc((6 + RESTORE_NEWLINES + 
strlen(this_module_name) + strlen(fstart)) * sizeof(char));
    strcpy(new_contents, "unit ");
    strcpy(new_contents + 5, this_module_name);
    for(int i = 0; i < RESTORE_NEWLINES; i ++){
      strcpy(new_contents + 5 + strlen(this_module_name) + i, "\n");
    }
    strcpy(new_contents + 5 + strlen(this_module_name) + RESTORE_NEWLINES, fstart);
    
    free(contents);
    
    //printf("of:%s\n", new_contents);

    for(int i = 0; i < import_queue_top; i ++){
      int SUB_RESTORE_NEWLINES = 0;
      char* file_text = find_module_file(".", import_queue[i]);
      if(file_text == NULL){
        file_only_error(where_imported_from_queue[i], "ModuleError", "Brie can only find modules in folders beneath the current directory.", "Can't find module %s", import_queue[i]);
      }
      //printf("%s\n", file_text);
      
      // we know that file_text starts with unit import_queue[i], so skip to \n
      char* file_start = file_text;
      while(*file_start != '\n') file_start ++;
      while(*file_start == '\n') {
        file_start ++;
        SUB_RESTORE_NEWLINES ++;
      }
      // TODO
      while(strncmp(file_start, "uses ", 5) == 0){
        file_start += 5;
        char* import_name_start = file_start;
        while(*file_start != '\n') file_start ++;
        
        char* import_name = (char*)malloc((file_start - import_name_start + 1) * sizeof(char));
        strncpy(import_name, import_name_start, file_start - import_name_start);
        import_name[file_start - import_name_start] = '\0';

        int do_add = strcmp(import_name, this_module_name);
        for(int i = 0; i < import_queue_top; i ++){
          if(!strcmp(import_name, import_queue[i])){
            do_add = 0;
            break;
          }
        }
        if(do_add){
          import_queue[import_queue_top] = import_name;
          import_queue_top++;
          if(import_queue_top >= import_queue_size){
            import_queue = realloc(import_queue, (import_queue_size *= 2) * sizeof(char*));
          }

          where_imported_from_queue[where_imported_from_queue_top] = import_queue[i];
          where_imported_from_queue_top++;
          if(where_imported_from_queue_top >= where_imported_from_queue_size){
            where_imported_from_queue = realloc(where_imported_from_queue, (where_imported_from_queue_size *= 2) * sizeof(char*));
          }
        }
        
        while(*file_start == '\n') { file_start++; SUB_RESTORE_NEWLINES ++; }
      }

      //printf("fs:%s\n", file_start);

      char* new_file_text = malloc((6 + RESTORE_NEWLINES + strlen(import_queue[i]) + strlen(file_start)) * sizeof(char));
      strcpy(new_file_text, "unit ");
      strcpy(new_file_text + 5, import_queue[i]);
      for(int j = 0; j < SUB_RESTORE_NEWLINES; j++){
        strcpy(new_file_text + 5 + strlen(import_queue[i]) + j, "\n");
      }
      strcpy(new_file_text + 5 + strlen(import_queue[i]) + SUB_RESTORE_NEWLINES, file_start);
      free(file_text);

      //printf("nf:%s\n", new_file_text); 

      char* old_new_contents = new_contents;
      new_contents = (char*)malloc((strlen(old_new_contents) + strlen(new_file_text) + 1) * sizeof(char));
      strcpy(new_contents, new_file_text);
      strcpy(new_contents + strlen(new_file_text), old_new_contents);
      free(old_new_contents);
      free(new_file_text);
      
      //printf("nc:%s\n", new_contents);
    }

    free(import_queue);
    free(where_imported_from_queue);

    contents = new_contents;
  }

  //printf("c:%s\n", contents);

  // standard workflow
  char* srcnamecpy = (char*)malloc(strlen(argv[1]) * sizeof(char));
  strcpy(srcnamecpy, argv[1]); // so it can be freed
  initScanner(contents, srcnamecpy);
  initChunk(&mainChunk);
  generateBytecode(parse());
  cleanupScanner();

#ifdef PREPRINT
  print_bytecode(&mainChunk);
#endif
  vm_loadchunk(&mainChunk);
  execute();
  vm_cleanup(); 
  free(scanner.source);
  
  return 0;
}