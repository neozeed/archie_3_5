#include <sys/types.h>
#include <stdio.h>
#include "source.h"

void
main(int argc, char *argv[])
{
  char sourcename[512];

  WAISSOURCE source = waissource_alloc();

  sprintf(sourcename,"%s.src", argv[1]);

  source = findsource(sourcename,
		      "/usr/local/wais/wais-sources/");
  if (source == NULL) { fprintf(stderr,"Error finding source.\n"); }
  else {
    printf("%s %s %s\n",source->server, source->name, source->service);
  }
}
