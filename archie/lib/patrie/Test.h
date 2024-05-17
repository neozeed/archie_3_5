#include <sys/types.h>


#define ARG_IS(str)  (strcmp(av[0], str) == 0)
#define NEXTDBL(dbl) do{if(av++,--ac<= 0||sscanf(av[0],"%lf",&dbl)!=1)usage();}while(0)
#define NEXTINT(i)   do{if(av++,--ac<=0||sscanf(av[0],"%d",&i)!=1)usage();}while(0)
#define NEXTLONG(i)  do{if(av++,--ac<=0||sscanf(av[0],"%ld",&i)!=1)usage();}while(0)
#define NEXTSTR(str) do{if(av++,--ac<=0)usage();else str=av[0];}while(0)
  

extern FILE *openFile(const char *name, const char *mode);
extern FILE *openTemp(const char *name, const char *mode);
extern char *getSearchKey(void);
extern int fpFork(FILE **rdFP, FILE **wrFP);
extern void printStrings(FILE *textFP, const char *key, unsigned long nhits, unsigned long start[]);
extern void printStarts(FILE *fp, unsigned long nhits, unsigned long start[]);
