#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lock.h"


#define LDIR  "."
#define LFILE "LOCK"


char *prog;


static int die(int ret)
{
  exit(ret);
  return 0;
}


int main(int ac, char **av)
{
  int i;
  
  prog = av[0];

  /*
   *  Open two shared locks, then try opening an exclusive lock.
   */

  printf("exclusive/exclusive/shared\n\n");

  for (i = 0; i <= 2; i++) {
    struct arch_lock *lock;
    
    switch (fork()) {
    case -1:
      fprintf(stderr, "%s: can't spawn child: ", prog); perror("fork");
      exit(1);

    case 0:
      switch (i) {
      case 0:                   /* shared lock */
        (lock = _archLockShared(LDIR, LFILE)) || die(1);
        printf("%d have shared lock.\n", i); sleep(2);
        _archUnlock(&lock) || die(1);
        printf("%d released shared lock.\n", i);
        exit(0);
        break;
      
      case 1:                   /* shared shared lock */
        (lock = _archLockShared(LDIR, LFILE)) || die(1);
        printf("%d have shared lock.\n", i); sleep(3);
        _archUnlock(&lock) || die(1);
        printf("%d released shared lock.\n", i);
        exit(0);
        break;

      case 2:                   /* exclusive lock */
        sleep(1);
        printf("%d try for exclusive lock.\n", i);
        (lock = _archLockExclusive(LDIR, LFILE)) || die(1);
        printf("%d have exclusive lock.\n", i); sleep(2);
        _archUnlock(&lock) || die(1);
        printf("%d released exclusive lock.\n", i);
        exit(0);
        break;
      }

    default:
      break;
    }
  }
  
  while (wait(0) != -1) {
    continue;
  }

  /*
   *  Open an exclusive lock, then try opening a shared lock.
   */

  printf("\nshared/exclusive\n");

  for (i = 0; i <= 1; i++) {
    struct arch_lock *lock;
    
    switch (fork()) {
    case -1:
      fprintf(stderr, "%s: can't spawn child: ", prog); perror("fork");
      exit(1);

    case 0:
      switch (i) {
      case 0:                   /* exclusive lock */
        (lock = _archLockExclusive(LDIR, LFILE)) || die(1);
        printf("%d have exclusive lock.\n", i); sleep(2);
        _archUnlock(&lock) || die(1);
        printf("%d released exclusive lock.\n", i);
        exit(0);
        break;

      case 1:                   /* shared lock */
        sleep(1);
        printf("%d try for shared lock.\n", i);
        (lock = _archLockShared(LDIR, LFILE)) || die(1);
        printf("%d have shared lock.\n", i); sleep(2);
        _archUnlock(&lock) || die(1);
        printf("%d released shared lock.\n", i);
        exit(0);
        break;
      }

    default:
      break;
    }
  }
  
  while (wait(0) != -1) {
    continue;
  }

  exit(0);
}
