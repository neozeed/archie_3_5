#ifndef STRHASH_H
#define STRHASH_H

#include "ansi_compat.h"


struct hash_elt
{
  const char *key; /* 0 means not in use. */
  void *val;
};


/*  
 *  Always keep one element free.  Table size should be a power of 2.
 */  
struct hash_table
{
  int nelts;
  int nused;
  struct hash_elt *helts;
};


extern int insert proto_((const char *key, void *val, struct hash_table *htab));
extern struct hash_table *new_hash_table proto_((int tabelts));
extern void *get_value proto_((const char *s, struct hash_table *htab));
extern void free_hash_table proto_((struct hash_table *htab, void (*freekey)(const char *),
                                    void (*freeval)(void *)));
extern void print_hash_table proto_((FILE *fp, struct hash_table *htab));

#endif
