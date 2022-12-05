#ifndef browns_h
#define browns_h

#include "value.h"

struct hash_list_node {
  struct hash_list_node* next;
  char* name;
  struct Value value;
};

#define HASHSIZE 101
typedef struct hash_list_node* hashtab[HASHSIZE];

struct hash_list_node* hash_lookup(hashtab, char*);
struct hash_list_node* hash_insert(hashtab, char*, struct Value);
struct hash_list_node* hash_remove(hashtab, char*);
hashtab*               clone_hash (hashtab);

#endif