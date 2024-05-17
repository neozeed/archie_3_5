/*
 *  Manage a cache of new additions to the strings file.
 */

#include <fcntl.h>
#include <errno.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "all.h"
#include "newstr.h"
#include "utils.h"


#define INIT_SZ_IDX  9
#define INIT_SBUF_SZ 1000

/*
 *  Try to maintain this load factor, or better (lower).
 */
#define DFLT_LOAD_FACTOR 0.9
#define MAX_LOAD_FACTOR  0.99
#define MIN_LOAD_FACTOR  0.01


struct str_buf {
  char *str;
  size_t len;                   /* total size of buffer */
  size_t next;                  /* next available location */
};


/*
 *  Create a new string buffer, which holds any new strings not found in the
 *  strings database.  Hash table elements contain offset into this buffer.
 */
static struct str_buf *new_str_buf(void)
{
  struct str_buf *s;

  if ( ! (s = malloc(sizeof *s))) {
    fprintf(stderr, "%s: new_str_buf: can't allocate %lu bytes for structure: ",
            prog, (unsigned long)sizeof *s);
    perror("malloc");
    return 0;
  }

  if ( ! (s->str = malloc(INIT_SBUF_SZ))) {
    fprintf(stderr, "%s: new_str_buf: can't allocate %lu bytes for buffer: ",
            prog, (unsigned long)INIT_SBUF_SZ); perror("malloc");
    free(s);
    return 0;
  }

  s->len = INIT_SBUF_SZ;
  s->next = 0;

  return s;
}


/*
 *  Append a string to the buffer, resizing it if nescessary.
 */
static int append_str_buf(char *s, struct str_buf *b, size_t *off)
{
  size_t l = strlen(s);

  while (l >= b->len - b->next) {
    char *new;
    
    if ( ! (new = realloc(b->str, 2 * b->len))) {
      fprintf(stderr, "%s: append_str_buf: can't allocate %lu bytes: ",
              prog, (unsigned long)(2 * b->len)); perror("realloc");
      return 0;
    }
    b->str = new;
    b->len *= 2;                /* double size of string buffer */
  }

  *off = b->next;
  strcpy(b->str + b->next, s);
  b->next += l + 1;

  return 1;
}


/*
 *  Return a pointer to a string, in the string buffer, at the given offset.
 *  Return a null pointer if the offset is beyond the buffer.
 */
static char *string_at(struct str_buf *b, size_t off)
{
  return off < b->len ? b->str + off : 0;
}


static void free_str_buf(struct str_buf **b)
{
  free((*b)->str);
  free(*b);
  *b = 0;
}


/*
 *  For 0 <= i <= 31, primes[i] is the first prime greater than 2^i.  (The
 *  hashing algorithm requires that the hash table contain a prime number of
 *  elements.)
 */
static unsigned long primes[] = {
  2, 2, 5, 11, 17, 37, 67, 131, 257, 521, 1031, 2053, 4099, 8209, 16411, 32771,
  65537, 131101, 262147, 524309, 1048583, 2097169, 4194319, 8388617, 16777259,
  33554467, 67108879, 134217757, 268435459, 536870923, 1073741827, 2147483659U
};


struct hash_elt {
  size_t str;                   /* offset of string within buffer */
  size_t off;                   /* offset of corresponding start */
};

struct hash_table {
  size_t szidx;                 /* index into `primes' giving current size */
  size_t nelts;                 /* cache for primes[htab->szidx] */
  
  size_t inuse;                 /* # of elements currently used */
  size_t rehash_lim;            /* grow the table when `inuse' exceeds this */

  struct hash_elt *elt;         /* pointer to elements */

  struct str_buf *sbuf;         /* pointer to strings */
};

/*
 *  One of `dir' or `htab' will be non-null, indicating which is being used.
 */
struct new_string_cache {
  struct hash_table *htab;      /* temporary hash table holding new strings */
  DBM *dbm;                     /* temporary DBM database holding new strings */
};


static int insert_hash_table(char *s, unsigned long off, struct hash_table **htab);


static int set_load_factor(struct hash_table *h, float lf)
{
  if (lf < MIN_LOAD_FACTOR) {
    fprintf(stderr, "%s: set_load_factor: load factor too small (< %f).\n",
            prog, MIN_LOAD_FACTOR);
    return 0;
  }

  if (lf > MAX_LOAD_FACTOR) {
    fprintf(stderr, "%s: set_load_factor: load factor too large (> %f).\n",
            prog, MAX_LOAD_FACTOR);
    return 0;
  }

  h->rehash_lim = h->nelts * lf;

  return 1;
}


static struct hash_table *create_hash_table(size_t idx)
{
  size_t i, mem;
  struct hash_table *h;
  
  if (idx >= sizeof primes / sizeof primes[0]) {
    fprintf(stderr, "%s: create_hash_table: no %lu'th element in primes table.\n",
            prog, (unsigned long)idx);
    return 0;
  }

  mem = sizeof *h;
  if ( ! (h = malloc(mem))) {
    fprintf(stderr, "%s: create_hash_table: can't allocate %lu bytes for hash table: ",
            prog, (unsigned long)mem); perror("malloc");
    return 0;
  }

  mem = sizeof *h->elt * primes[idx];
  if ( ! (h->elt = malloc(mem))) {
    fprintf(stderr, "%s: create_hash_table: can't allocate %lu bytes for hash table elements: ",
            prog, (unsigned long)mem); perror("malloc"); free(h);
    return 0;
  }

  h->szidx = idx;
  h->nelts = primes[idx];

  if ( ! (h->sbuf = new_str_buf())) {
    fprintf(stderr, "%s: create_hash_table: can't create string buffer.\n", prog);
    free(h->elt); free(h);
    return 0;
  }

  for (i = 0; i < h->nelts; i++) {
    h->elt[i].str = (size_t)-1;
    h->elt[i].off = 0;
  }

  h->inuse = 0;
  set_load_factor(h, DFLT_LOAD_FACTOR);
  
  return h;
}


static void free_hash_table(struct hash_table **htab)
{
  struct hash_table *h = *htab;

  free(h->elt);
  free_str_buf(&h->sbuf);
  free(h);
  *htab = 0;
}


/*
 *  Hash function based on Chris Torek's.
 */
static unsigned long hash0(char *s)
{
  unsigned char *p;
  unsigned long h = 0;
  
  for (p = (unsigned char *)s; *p; p++) {
    h = h * 33 + *p;
  }

  return h;
}


/*
 *  Hash function from Gonnet and Baeza-Yates.  Only the constant differs from
 *  Torek's.
 */
static unsigned long hash1(char *s)
{
  unsigned char *p;
  unsigned long h = 0;
  
  for (p = (unsigned char *)s; *p; p++) {
    h = h * 131 + *p;
  }

  return h;
}


/*
 *  Create a new hash table twice the size of the old one, and fill it with
 *  the contents of the old table.  Free the old table.
 */
static int expand_hash_table(struct hash_table **htab)
{
  size_t i;
  struct hash_table *nh, *oh = *htab;
  
  if ( ! (nh = create_hash_table(oh->szidx + 1))) {
    fprintf(stderr, "%s: expand_hash_table: can't create new hash table.\n", prog);
    return 0;
  }

  for (i = 0; i < oh->nelts; i++) {
    if (string_at(oh->sbuf, oh->elt[i].str) &&
        ! insert_hash_table(string_at(oh->sbuf, oh->elt[i].str), oh->elt[i].off, &nh)) {
      fprintf(stderr, "%s: expand_hash_table: error inserting into new hash table\n", prog);
      free_hash_table(&nh);
      return 0;
    }
  }

  free_hash_table(&oh);

  *htab = nh;

  return 1;
}


/*
 *  Insert `s', and its associated offset, `off', into the hash table.  We
 *  assume that it's not already there.
 *  
 *  Return 0 if there's an error inserting a new item, otherwise return 1.
 *  
 *  bug: we should have the search routine return the insertion position, if
 *  it can't find the string.
 */
static int insert_hash_table(char *s, unsigned long off, struct hash_table **htab)
{
  size_t i, n;
  struct hash_table *h = *htab;
  
  if (h->inuse >= h->rehash_lim) {
    if ( ! expand_hash_table(&h)) {
      return 0;
    }
  }

  n = hash0(s) % h->nelts;
  i = 1 + hash1(s) % (h->nelts - 1); /* 1 <= i <= nelts-1 */
  while (string_at(h->sbuf, h->elt[n].str) &&
         strcmp(string_at(h->sbuf, h->elt[n].str), s) != 0) {
    n = (n + i) % h->nelts;
  }

  if ( ! append_str_buf(s, h->sbuf, &h->elt[n].str)) {
    fprintf(stderr, "%s: insert: error appending new string to buffer.\n", prog);
    return 0;
  }
  h->elt[n].off = off;
  h->inuse++;

  *htab = h;

  return 1;
}


/*
 *  Search for `s' in the hash table.  If it's found set `*off' to its offset
 *  in the strings file and set `*found' to 1.  If it's not found set `*found'
 *  to 0.
 */
static int search_hash_table(struct hash_table *h, char *s, unsigned long *off, int *found)
{
  size_t i, n;

  n = hash0(s) % h->nelts;
  i = 1 + hash1(s) % (h->nelts - 1); /* 1 <= i <= nelts-1 */
  while (string_at(h->sbuf, h->elt[n].str) &&
         strcmp(string_at(h->sbuf, h->elt[n].str), s) != 0) {
    n = (n + i) % h->nelts;
  }

  *off = h->elt[n].off;
  *found = string_at(h->sbuf, h->elt[n].str) != 0;

  return 1;
}


static int get_key(FILE *fp, size_t ksz, char *key, size_t *klen, int *eof)
{
  *eof = 0;
  
  if ( ! fgets(key, ksz, fp)) {
    if (ferror(fp)) {
      fprintf(stderr, "%s: get_key: error reading key: ", prog); perror("fgets");
      return 0;
    }

    *eof = 1;
    return 0;
  } else {
    char *nl = strchr(key, KEYSEP_CH);
    
    /*
     *  If there's no newline in the buffer either the string is too long or
     *  the file has no trailing newline.
     */
    
    if ( ! nl) {
      if (strlen(key) == ksz - 1) {
        fprintf(stderr, "%s: get_key: key `%s' is too long.\n", prog, key);
      } else {
        fprintf(stderr, "%s: get_key: `%s' is not followed by a key separator.\n", prog, key);
      }
      fprintf(stderr, "%s: get_key: this key occurred before offset %ld.\n",
              prog, ftell(fp));
      return 0;
    }

    if (key[0] == '\0') {
      fprintf(stderr, "%s: get_key: read an empty key near offset %ld.\n",
              prog, ftell(fp));
      return 0;
    }

    *nl = '\0';
    *klen = nl - key;
    
    return 1;
  }
}


/*
 *  Insert, into the hash table, the strings in the file given by `fp'.
 *  `start' is the offset, in the strings file, of the first string to be
 *  read.  (We assume that the file pointer is positioned at the first
 *  character of the first unindexed string.)
 */
static int init_hash_table(struct hash_table **htab, FILE *fp, long start)
{
  char key[1024];               /* bug: fixed length */
  int eof = 0;
  long off;
  size_t klen;
  
  off = start;

  while (get_key(fp, sizeof key, key, &klen, &eof)) {
    /*
     *  Store (key, off) in the hash table.
     */

    if ( ! insert_hash_table(key, off, htab)) {
      fprintf(stderr, "%s: init_hash_table: error inserting key into hash table.\n", prog);
      return 0;
    }

    off += klen + 1;
  }

  if ( ! eof) {
    fprintf(stderr, "%s: init_hash_table: error reading key.\n", prog);
    return 0;
  }

  return 1;
}


/*  
 *  Create a temporary DBM file to keep track of new strings added, but not
 *  yet incorporated into the index.  Unlink the .dir and .pag files in case
 *  we crash.
 *  
 *  Normally, the DBM file will be used only if we can't create a hash table.
 *  
 *  bug: These routines probably aren't very useful.  If the hash table is
 *  going to fail, it will probably be while attempting a resize; too late to
 *  fall back to the DBM files.  (Well, we could, but I don't want to code
 *  that right now...)
 */
static DBM *create_dbm(const char *dir)
{
  DBM *dbm;
  char *tdb = 0, *tdbdir = 0, *tdbpag = 0;
    
  if ( ! ((tdb = _archMakePath(dir, "/", APPEND_FILE)) &&
          (tdbdir = _archMakePath(tdb, "", ".dir"))    &&
          (tdbpag = _archMakePath(tdb, "", ".pag")))) {
    fprintf(stderr, "%s: create_dbm: can't build paths to DBM files.\n", prog);
    if (tdb) free(tdb);
    if (tdbdir) free(tdbdir);
    return 0;
  }

  if ( ! (dbm = dbm_open(tdb, O_RDWR | O_CREAT | O_EXCL, 0600))) {
    fprintf(stderr, "%s: create_dbm: can't create temporary database file `%s': ",
            prog, tdb); perror("dbm_open");
    free(tdb); free(tdbdir); free(tdbpag);
    return 0;
  }

  unlink(tdbdir); unlink(tdbpag);
  free(tdb); free(tdbdir); free(tdbpag);

  return dbm;
}


/*
 *  Insert, into the DBM files, the strings in the file given by `fp'.
 *  `start' is the offset, in the strings file, of the first string to be
 *  read.  (We assume that the file pointer is positioned at the first
 *  character of the first unindexed string.)
 */
static int init_dbm(DBM *dbm, FILE *fp, long start)
{
  char key[1024];               /* bug: fixed length */
  datum kdat, sdat;
  int eof;
  long off;
  size_t klen;
  
  off = start;

  while (get_key(fp, sizeof key, key, &klen, &eof)) {
    /*
     *  Store (key, off) in the DBM file.  It's an error if `key' already
     *  exists.
     */

    kdat.dptr = key;          kdat.dsize = klen;
    sdat.dptr = (char *)&off; sdat.dsize = sizeof off;

    switch (dbm_store(dbm, kdat, sdat, DBM_INSERT)) {
    case 0:                     /* okay */
      break;

    case 1:                     /* key already exists! (shouldn't occur) */
      fprintf(stderr,
              "%s: init_dbm: *** `%s' already in temporary database! ***\n",
              prog, key);
      return 0;
      break;

    default:
      fprintf(stderr,
              "%s: init_dbm: error inserting `%s' into temporary database.\n",
              prog, key); perror("dbm_store");
      return 0;
      break;
    }

    off += klen + 1;
  }

  if ( ! eof) {
    fprintf(stderr, "%s: init_dbm: error reading key.\n", prog);
    return 0;
  }

  return 1;
}


struct new_string_cache *_archNewStringCache(const char *dbdir)
{
  struct new_string_cache *c;

  if ( ! (c = malloc(sizeof *c))) {
    fprintf(stderr, "%s: _archNewStringCache: can't allocate cache structure: ", prog);
    perror("malloc");
    return 0;
  }

  c->dbm = 0;

  /*
   *  Try creating a hash table.  If that fails fall back to the DBM file.
   */

  if ( ! (c->htab = create_hash_table(INIT_SZ_IDX))) {
    fprintf(stderr, "%s: _archNewStringCache: can't create hash table; trying DBM file.\n",
            prog);
    if ( ! (c->dbm = create_dbm(dbdir))) {
      fprintf(stderr, "%s: _archNewStringCache: can't create DBM file for new strings.\n",
              prog); free(c);
      return 0;
    }
  }

  return c;
}


/*
 *  Initialize the cache with the contents of of the strings file, starting at
 *  `start'.
 */
int _archInitStringCache(struct new_string_cache *c, FILE *sfp, size_t start)
{
  long opos;
  
  opos = ftell(sfp);
  
  if (fseek(sfp, (long)start, SEEK_SET) == -1) {
    fprintf(stderr, "%s: _archInitStringCache: can't seek to offset %ld in strings file: ",
            prog, (long)start); perror("fseek");
    (void)fseek(sfp, opos, SEEK_SET); /* hope it works... */
    return 0;
  }

  if (c->htab && ! init_hash_table(&c->htab, sfp, start)) {
    fprintf(stderr, "%s: _archInitStringCache: can't load unindexed text into hash table.\n",
            prog);
    (void)fseek(sfp, opos, SEEK_SET);
    return 0;
  } else if ( ! init_dbm(c->dbm, sfp, start)) {
    fprintf(stderr, "%s: _archInitStringCache: can't load unindexed text into DBM database.\n",
            prog);
    (void)fseek(sfp, opos, SEEK_SET);
    return 0;
  }

  if (fseek(sfp, opos, SEEK_SET) == -1) {
    fprintf(stderr, "%s: _archInitStringCache: can't seek to original position in strings file: ",
            prog); perror("fseek");
    return 0;
  }

  return 1;
}


int _archFreeStringCache(struct new_string_cache **cache)
{
  struct new_string_cache *c = *cache;

  if (c->htab) {
    free_hash_table(&c->htab);
  }

  if (c->dbm) {
    dbm_close(c->dbm);
    c->dbm = 0;
  }

  free(c);
  *cache = 0;

  return 1;
}


/*
 *  Add `key' to the cache.  Its corresponding offset is given by `off'.
 */
int _archAddKeyToStringCache(struct new_string_cache *c, const char *key, unsigned long off)
{
  if (c->htab) {
    if ( ! insert_hash_table(key, off, &c->htab)) {
      fprintf(stderr, "%s: _archAddKeyToStringCache: error inserting key into hash table.\n",
              prog);
      return 0;
    }
  } else {
    datum kdat, sdat;

    kdat.dptr = key;
    kdat.dsize = strlen(key);

    sdat.dptr = (char *)&off;
    sdat.dsize = sizeof off;

    errno = 0;
    if (dbm_store(c->dbm, kdat, sdat, DBM_INSERT) < 0) {
      fprintf(stderr, "%s: _archAddKeyToStringCache: can't store new key and offset: ", prog);
      perror("dbm_store");
      return 0;
    }
  }

  return 1;
}


/*
 *  If `key' is in the cache set `*found' to 1 and set `*off' to its offset,
 *  otherwise set `*found' to 0.
 */
int _archKeyInStringCache(struct new_string_cache *c,
                          const char *key,
                          int *found,
                          unsigned long *off)
{
  if (c->htab) {
    return search_hash_table(c->htab, key, off, found);
  } else {
    datum kdat, sdat;

    kdat.dptr = key;
    kdat.dsize = strlen(key);

    sdat = dbm_fetch(c->dbm, kdat);

    if (sdat.dptr) {
      memcpy(off, sdat.dptr, sizeof *off);
      *found = 1;
      return 1;
    }

    *found = 0;
    return 1;
  }
}


/*
 *  Set the maximum load factor for the hash table.
 */
int _archStringCacheLoadFactor(struct new_string_cache *c, float lf)
{
  if ( ! c->htab) {
    fprintf(stderr, "%s: _archStringCacheLoadFactor: value ignored -- not using hash table.\n",
            prog);
    return 0;
  }
  
  return set_load_factor(c->htab, lf);
}

