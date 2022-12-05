// modified from section 6.6 of The C Programming Language

#include <stdlib.h>
#include <string.h>

#include "hash.h"

unsigned hash(char* s) {
  unsigned hashval;
  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;
  return hashval % HASHSIZE;
}

struct hash_list_node *hash_lookup(hashtab h, char* s)
{
    struct hash_list_node* np;
    for (np = h[hash(s)]; np != NULL; np = np->next)
      if (strcmp(s, np->name) == 0)
        return np; /* found */
    return NULL; /* not found */
}

char* dupstr(char* s) { // ignore collision with POSIX
  char* p;
  p = (char *) malloc(strlen(s) + 1);
  if (p != NULL)
      strcpy(p, s);
  return p;
}

struct hash_list_node* hash_insert(hashtab h, char* name, struct Value defn)
{
    struct hash_list_node* np;
    unsigned hashval;
    if ((np = hash_lookup(h, name)) == NULL) {
      np = (struct hash_list_node *) malloc(sizeof(*np));
      if (np == NULL || (np->name = dupstr(name)) == NULL)
        return NULL;
  
      hashval = hash(name);
      np->next = h[hashval];

      h[hashval] = np;
    }

    np->value = defn;

    return np;
}

struct hash_list_node* hash_remove(hashtab h, char* name) {
  struct hash_list_node* np;
  struct hash_list_node* prevp = NULL;
  for (np = h[hash(name)]; np != NULL; np = np->next) {
    if (strcmp(name, np->name) == 0) {
      if (prevp == NULL) {
        h[hash(name)] = np->next;
      }
      else prevp->next = np->next;
      free(np);
      break;
    }
    prevp = np;
  }
  
  return np;
}

hashtab* clone_hash(hashtab table) {
  hashtab* cloned = (hashtab*)calloc(1, sizeof(hashtab));
  for(int i = 0; i < HASHSIZE; i ++) {
    struct hash_list_node* lastnode = NULL;
    for(struct hash_list_node* np = table[i]; np != NULL; np = np->next) {
      struct hash_list_node* newnode = (struct hash_list_node*)malloc(sizeof(struct hash_list_node));
      memcpy(newnode, np, sizeof(struct hash_list_node));

      if(lastnode == NULL) {
        (*cloned)[i] = newnode;
        lastnode = (*cloned)[i];
      } else {
        lastnode->next = newnode;
        lastnode = lastnode->next;
      }
    }
  }

  return cloned;
}