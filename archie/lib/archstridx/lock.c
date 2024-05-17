#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "all.h"
#include "lock.h"
#include "utils.h"


struct arch_lock {
  FILE *fp;
  struct flock lock;
};


static void init_lock(short type, struct arch_lock *lock)
{
  lock->fp = 0;

  lock->lock.l_type = type;
  lock->lock.l_start = 0;
  lock->lock.l_whence = SEEK_SET;
  lock->lock.l_len = 0;
}


static void free_lock(struct arch_lock **lock)
{
  struct arch_lock *l = *lock;
  
  if (l->fp) fclose(l->fp);
  free(l);
  *lock = 0;
}


static struct arch_lock *new_lock(char *dir, char *file, short type)
{
  struct arch_lock *l;

  if ( ! (l = malloc(sizeof *l))) {
    fprintf(stderr, "%s: new_lock: can't allocate new lock structure: ",
            prog); perror("malloc");
    return 0;
  }
  
  init_lock(type, l);
  
  if ( ! _archFileOpen(dir, file, "w+", 0, &l->fp) || ! l->fp) {
    fprintf(stderr, "%s: new_lock: can't open lock file: ",
            prog); perror("_archFileOpen");
    free_lock(&l);
    return 0;
  }

  return l;
}


struct arch_lock *_archLockShared(char *dir, char *file)
{
  struct arch_lock *l;

  if ( ! (l = new_lock(dir, file, F_RDLCK))) {
    fprintf(stderr, "%s: _archLockShared: can't create new lock.\n", prog);
    return 0;
  }
  
  if (fcntl(fileno(l->fp), F_SETLKW, &l->lock) == -1) {
    fprintf(stderr, "%s: _archLockShared: can't lock file: ", prog);
    perror("fcntl"); free_lock(&l);
    return 0;
  }

  return l;
}


struct arch_lock *_archLockExclusive(char *dir, char *file)
{
  struct arch_lock *l;

  if ( ! (l = new_lock(dir, file, F_WRLCK))) {
    fprintf(stderr, "%s: _archLockExclusive: can't create new lock.\n", prog);
    return 0;
  }
  
  if (fcntl(fileno(l->fp), F_SETLKW, &l->lock) == -1) {
    fprintf(stderr, "%s: _archLockExclusive: can't lock file: ", prog);
    perror("fcntl"); free_lock(&l);
    return 0;
  }

  return l;
}


int _archUnlock(struct arch_lock **lock)
{
  struct arch_lock *l = *lock;

  l->lock.l_type = F_UNLCK;

  if (fcntl(fileno(l->fp), F_SETLKW, &l->lock) == -1) {
    fprintf(stderr, "%s: _archUnlock: can't unlock file: ", prog);
    perror("fcntl"); free_lock(lock);
    return 0;
  }

  free_lock(lock);

  return 1;
}
