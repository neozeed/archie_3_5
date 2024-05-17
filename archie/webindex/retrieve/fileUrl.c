#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

typedef struct {
  char *url;
  char *file;
} fileList;

typedef struct {

  char *filename;
  fileList *list;
  int num, max;
  
} fileUrlObject;


static int fileListCmp( void *a , void *b )
{
  return strcasecmp(((fileList*)b)->url, ((fileList*)a)->url );
}

extern int errno;

fileUrlObject *fileUrlNew()
{
  fileUrlObject *f;

  f = (fileUrlObject *)malloc(sizeof(fileUrlObject));
  if ( f != NULL ) {
    f->filename = NULL;
    f->list = (fileList*)malloc(sizeof(fileList)*10);
    if ( f->list == NULL ) {
      free(f);
      return NULL;
    }
    f->num = 0;
    f->max = 10;
  }
  return f;
}



void fileUrlFree(fileUrlObject *file)
{
  free(file);
}


int fileUrlRead(fileUrlObject *file, char *filename, int present )
{

  char buffer[1000];
  char url[1000], f[1000];
  
  FILE *fp;
  
  file->filename = (char *)malloc(sizeof(char)*(strlen(filename)+1));
  if ( file->filename == NULL )
  return 0;

  fp = fopen(filename, "r");
  if ( fp == NULL ) {
    if ( errno != ENOENT || present ) {
      return 0;
    }
    else {
      return 1;
    }
  }

  while ( fgets(buffer,1000,fp) != NULL ) {
    char *t;

    t = buffer;

    sscanf(buffer," %s %s ", url, f);

    if ( file->num == file->max ) {
      fileList *tmp;

      tmp = (fileList*)realloc(file->list, (file->max+10)* sizeof(fileList));
      if ( tmp == NULL ) {
        fclose(fp);
        return 0;
      }
      file->max += 10;
      file->list = tmp;
    }

    file->list[file->num].url = (char*)malloc((strlen(url)+1)*sizeof(char));
    file->list[file->num].file = (char*)malloc((strlen(f)+1)*sizeof(char));

    if ( file->list[file->num].url  == NULL ||
         file->list[file->num].file == NULL    )  {

      fclose(fp);
      return 0;
    }

    strcpy(file->list[file->num].url, url);
    strcpy(file->list[file->num].file, f);

    file->num++;
  }

  qsort(file->list, file->num, sizeof(fileList), fileListCmp);
        
  fclose(fp);
  return 1;
}



int fileUrlFind( fileUrlObject *file, char *url, char **filename )
{

  int i;
  int l,len;
  char *tmp, *s,*t;
  
  for ( i = 0; i < file->num; i++ ) {
    len = strlen(file->list[i].url);

    if ( strncasecmp(url, file->list[i].url,len) == 0 ) {

      if ( file->list[i].url[len] == '\0' || file->list[i].url[len] == '/'  ) {
        l = strlen(file->list[i].file)+strlen(url)-len + 2;

        tmp = (char*)malloc(l*sizeof(char));
        if ( tmp == NULL ) {
          return 0;
        }
        sprintf(tmp, "%s/%s",file->list[i].file, &url[len]);
        s = tmp;
        t = tmp;
        for ( s = tmp, t = tmp; *s != '\0' ; s++ ) {
          if ( *s != '/' || *(s+1) != '/' ) {
            *t = *s;
            t++;
          }
        }
        *t = '\0';
        *filename = tmp;
        return 1;
      }
          
    }
  }

  *filename = NULL;
  return 1;
}

