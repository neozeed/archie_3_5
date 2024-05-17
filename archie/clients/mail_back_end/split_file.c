/*
  Usage:

    split_file -s <max-part-size> -f <fname-base> [-n]

  By default an integral number of text lines will be written to each "split
  file".  That is, <max-part-size> bytes, plus how ever may characters it takes
  to commplete the current text line.

  The `-n' option specifies that exactly <max-part-size> bytes will be written to
  each "split file".

  <fname-base> is the base part of the output file name; a `.<n>' will be appended
  to the actual name, where <n> is a number greater than or equal to zero.

*/


#include <fcntl.h>
#include <limits.h>
#include <malloc.h>
#ifndef __GNUC__
#include <memory.h> /* mempcy is a GNU built-in & non-ANSI definition conflicts */
#endif
#include <stdio.h>
#include <string.h>
#include "ansi_compat.h"
/* #include "misc_ansi_defs.h"*/


#define MIN_SPLIT_SIZE 1024  /* minimum size of a split file */
#define LSIZE MIN_SPLIT_SIZE /* limits line length in line-at-a-time mode */


extern const char *tail PROTO((const char *path));
extern int fill_file PROTO((FILE *ifp, FILE *ofp, int size, int line_at_a_time));
extern void usage PROTO((void));


const char *prog;


int main(ac, av)
  int ac;
  char **av;
{
  FILE *ofp;
  char *fname;
  char *fnbase = (char *)0;
  int filenum = 0;
  int eof = 0;
  int s = INT_MIN;
  int line_at_a_time = 1;

  prog = tail(av[0]);
  while(av++, --ac)
  {
    if (av[0][0] != '-')
    {
      usage();
    }
    else
    {
      switch (av[0][1])
      {
      case 'f':
        av++, --ac;
        fnbase = av[0];
        break;

      case 's':
        av++, --ac;
        s = atoi(av[0]);
        break;

      case 'n':
        line_at_a_time = 0;
        break;
        
      default:
        usage();
      }
    }
  }
  if ( ! fnbase || s == INT_MIN)
  {
    usage();
  }
  if (s < MIN_SPLIT_SIZE)
  {
    fprintf(stderr, "%s: you must specify a size greater than or equal to `%d'.\n",
            prog, MIN_SPLIT_SIZE);
    usage();
  }
  if ( ! (fname = malloc((unsigned)strlen(fnbase) + 12)))
  {
    fprintf(stderr, "%s: error malloc()ing %d bytes for output file names: ", prog,
            strlen(fnbase) + 12);
    perror("");
    exit(1);
  }

  umask(077);
  while ( ! eof)
  {
    int n;

    filenum++;
    sprintf(fname, "%s.%d", fnbase, filenum);
    if ( ! (ofp = fopen(fname, "w")))
    {
      fprintf(stderr, "%s: error opening `%s' write-only: ", prog, fname); perror("");
      fprintf(stderr, "%s: exiting.\n", prog);
      free(fname);
      exit(1);
    }
    
    switch (n = fill_file(stdin, ofp, s, line_at_a_time))
    {
    case -1:
      fprintf(stderr, "%s: error -- exiting.\n", prog);
      fclose(ofp);
      free(fname);
      exit(1);

    case 0:
      /*
        End of file and nothing was written, so we can toast the newly
        created file.
      */
      eof = 1;
      unlink(fname);
      break;

    default:
      if (n < s) /* eof */
      {
        eof = 1;
      }
      break;
    }

    fclose(ofp);
  }
  free(fname);
  exit(0);
}


int fill_file(ifp, ofp, size, line_at_a_time)
  FILE *ifp;
  FILE *ofp;
  int size;
  int line_at_a_time;
{
  int nwritten = 0;

  if (line_at_a_time)
  {
    char line[LSIZE + 1];

    while (nwritten < size)
    {
      int slen;

      line[LSIZE - 1] = '\0';
      if (fgets(line, sizeof line, ifp))
      {
        if (line[LSIZE - 1] != '\0' && line[LSIZE - 1] != '\n')
        {
          fprintf(stderr, "%s: line too long! (> %d bytes)\n", prog, LSIZE);
          return -1;
        }
        slen = strlen(line);
        if (fwrite(line, 1, slen, ofp) == slen)
        {
          nwritten += slen;
        }
        else
        {
          fprintf(stderr, "%s: error fwrite()ing %d bytes: ", prog, slen); perror("");
          return -1;
        }
      }
      else
      {
        if (feof(ifp))
        {
          return nwritten;
        }
        else
        {
          fprintf(stderr, "%s: error reading one line: ", prog); perror("");
          return -1;
        }
      }
    }
  }
  else
  {
    while (nwritten < size)
    {
      int c;

      if ((c = getc(ifp)) != EOF)
      {
        if (putc(c, ofp) == c)
        {
          nwritten++;
        }
        else
        {
          fprintf(stderr,"%s: error writing one character: ", prog); perror("");
          return -1;
        }
      }
      else
      {
        if (feof(ifp))
        {
          return nwritten;
        }
        else
        {
          fprintf(stderr, "%s: error reading a character: ", prog); perror("");
          return -1;
        }
      }
    }
  }

  return nwritten; /* nwritten == size */
}


const char *tail(path)
  const char *path;
{
  const char *p = strrchr(path, '/');
  return p ? p + 1 : path;
}


void usage()
{
  fprintf(stderr, "Usage: %s -s <max-part-size> -f <output-fname-base> [-n]\n", prog);
  exit(1);
}
