#include <errno.h>

#define ARG_IS(str)  (strcmp(av[0], str) == 0)
#define NEXTDBL(dbl) do{if(av++,--ac<= 0||sscanf(av[0],"%lf",&dbl)!=1)usage();}while(0)
#define NEXTINT(i)   do{if(av++,--ac<=0||sscanf(av[0],"%d",&i)!=1)usage();}while(0)
#define NEXTUINT(i)   do{if(av++,--ac<=0||sscanf(av[0],"%u",&i)!=1)usage();}while(0)
#define NEXTLONG(i)  do{if(av++,--ac<=0||sscanf(av[0],"%ld",&i)!=1)usage();}while(0)
#define NEXTULONG(i)  do{if(av++,--ac<=0||sscanf(av[0],"%lu",&i)!=1)usage();}while(0)
#define NEXTSTR(str) do{if(av++,--ac<=0)usage();else str=av[0];}while(0)
  
#define ASSERT(cond) \
do { \
  errno = 0; \
  if ( ! (cond)) { \
    fprintf(stderr, "%s:%d: failed assertion `%s'.\n", __FILE__, __LINE__, #cond); \
    if (errno != 0) perror("system call"); \
    abort(); \
    exit(1); \
  } \
} while (0)
