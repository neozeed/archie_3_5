#include <sys/stat.h>
#include <stdio.h>


int main()
{
  struct stat sbuf ;

  if (fstat(0, &sbuf) == -1)
  {
    perror("fstat");
    exit(1);
  }

  if (S_ISSOCK(sbuf.st_mode))
  {
    printf("Got a socket.\n");
  }
  else
  {
    printf("Ceci n'est pas un pipe (sockette?)\n");
    printf("st_mode = %07o\n", sbuf.st_mode);
  }
  exit(0);
}
