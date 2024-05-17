#ifdef __STDC__
# include <stdlib.h>
#endif
#include <stdio.h>
#include "defs.h"
#include "error.h"
#include "strhash.h"
#include "all.h"


/*
 *
 *
 *                             Internal routines
 *
 *
 */

static int table_index proto_((const char *key, struct hash_table *htab));
static unsigned int hval proto_((const char *str, int tabsz));


static int table_full(htab)
  struct hash_table *htab;
{
  return htab->nused == htab->nelts - 1;
}


/*  
 *  Return the location of `key' in `htab'.  If it isn't found and there is
 *  room for another element, then return the location at which it would be
 *  inserted.  If there is no room, return -1.
 */  
static int table_index(key, htab)
  const char *key;
  struct hash_table *htab;
{
  unsigned int h;

  h = hval(key, htab->nelts);
  while (htab->helts[h].key && strcmp(key, htab->helts[h].key) != 0)
  {
    h = (h + 1) % htab->nelts;
  }
  if ( ! htab->helts[h].key && table_full(htab))
  {
    return -1;
  }

  return (int)h;
}


static unsigned int hval(str, tabsz)
  const char *str;
  int tabsz;
{
  unsigned char *p;
  unsigned int h;

  h = 0;
  for (p = (unsigned char *)str; *p; p++)
  {
    h = h * 33 + *p;
  }
  return h % tabsz;
}


/*
 *
 *
 *                             External routines
 *
 *
 */


/*  
 *  Insert `val' into hash table.  If the element already exits
 *  we fail.
 *
 *  In most cases the objects pointed to by `key' and `val' will
 *  have been duplicated.
 */  
int insert(key, val, htab)
  const char *key;
  void *val;
  struct hash_table *htab;
{
  unsigned int h;

  if ((h = table_index(key, htab)) == -1)
  {
    efprintf(stderr, "%s: insert: table_index() failed.\n", logpfx());
    return 0;
  }

  if (htab->helts[h].key)
  {
    efprintf(stderr, "%s: insert: `%s' already in table.\n", logpfx(), key);
    return 0;
  }
  else
  {
    htab->helts[h].key = key;
    htab->helts[h].val = val;
    return 1;
  }
}


/*  
 *  Allocate space for, and return a pointer to, a new hash table.
 *
 *  `tabelts' is the size of the table.  The actual size must be a non-zero
 *  power of two.  The number of available elements will be one less than
 *  `tabelts'.
 */  
struct hash_table *new_hash_table(tabelts)
  int tabelts;
{
  int i;
  int n;
  struct hash_elt *he;
  struct hash_table *ht;
  
  if (tabelts <= 0)
  {
    efprintf(stderr, "%s: new_hash_table: number of elements, %d, must be >= 0.\n",
             logpfx(), tabelts);
    return 0;
  }

  if ((tabelts & (tabelts - 1)) != 0)
  {
    efprintf(stderr, "%s: new_hash_table: number of elements, %d, must be a power of two.\n",
             logpfx(), tabelts);
    return 0;
  }

  n = sizeof(struct hash_table);
  if ( ! (ht = malloc(n)))
  {
    efprintf(stderr, "%s: new_hash_table: can't allocate %d bytes for table.\n",
             logpfx(), n);
    return 0;
  }

  n = tabelts * sizeof(struct hash_elt);
  if ( ! (he = malloc(n)))
  {
    efprintf(stderr, "%s: new_hash_table: can't allocate %d bytes for elements.\n",
             logpfx(), n);
    free(ht);
    return 0;
  }

  for (i = 0; i < tabelts; i++)
  {
    he[i].key = (const char *)0;
    he[i].val = (void *)0;
  }
  
  ht->nelts = tabelts;
  ht->nused = 0;
  ht->helts = he;

  return ht;
}


/*  
 *  Return the value corresponding to `key'.  If the key is not found a null
 *  pointer will be returned.
 */  
void *get_value(key, htab)
  const char *key;
  struct hash_table *htab;
{
  int h;

  if ((h = table_index(key, htab)) == -1)
  {
    efprintf(stderr, "%s: get_value: `%s' not found.\n", logpfx(), key);
    return 0;
  }

  /* Note: it still may not have been found. */
  return htab->helts[h].val;
}


void free_hash_table(htab, freekey, freeval)
  struct hash_table *htab;
  void (*freekey) proto_((const char *));
  void (*freeval) proto_((void *));
{
  if ( ! htab)
  {
    efprintf(stderr, "%s: free_hash_table: null pointer argument.\n", logpfx());
  }
  else
  {
    int i;
    
    for (i = 0; i < htab->nelts; i++)
    {
      if (freekey && htab->helts[i].key)
      {
        freekey(htab->helts[i].key);
      }
      if (freeval && htab->helts[i].val)
      {
        freeval(htab->helts[i].val);
      }
    }
    free(htab->helts);
    free(htab);
  }
}


void print_hash_table(fp, htab)
  FILE *fp;
  struct hash_table *htab;
{
  int i;
  
  for (i = 0; i < htab->nelts; i++)
  {
    if (htab->helts[i].key)
    {
      printf("#%d  Key: `%s', Value: %p\n",
             i, htab->helts[i].key, htab->helts[i].val);
    }
  }
}
