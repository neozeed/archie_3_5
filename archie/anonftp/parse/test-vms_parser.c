#include <stddef.h>
#ifdef NULL
#   undef NULL
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "utils.h"
#include "db2v.h"
#include "parser_file.h"
#include "archie_inet.h"
#include "header.h"
#include "host_db.h"
#include "error.h"

#ifdef __STDC__

extern int print_parser_info(FILE *fp, parser_entry_t *pe, char *name, int n) ;
extern void usage(void) ;

#else

extern int print_parser_info(/* FILE *fp, parser_entry_t *pe, char *name, int n */) ;
extern void usage(/* void */) ;

#endif

#define	 MAX_LINE_LEN	512


char *prog ;
int verbose = 1 ;


int
main(ac, av)
  int ac ;
  char **av ;
{
  header_t h ;
  int ignore_header = 0 ;
  int len ;
  long n = 0 ;
  parser_entry_t pe ;

  prog = tail(av[0]) ;

  while(av++, --ac)
  {
    if(av[0][0] != '-')
    {
      usage() ;
    }
    else
    {
      switch(av[0][1])
      {
      case 'h':
        ignore_header = 1 ;
        break ;

      default:
        usage() ;
        break ;
      }
    }
  }

  if( ! ignore_header)
  {
    if(read_header(stdin, &h, (u32 *)0, 0,0) != A_OK)
    {
      fprintf(stderr, "%s: error from read_header().\n", prog) ;
      exit(1) ;
    }
    if(write_header(stdout, &h, (u32 *)0, 0) != A_OK)
    {
      fprintf(stderr, "%s: error from write_header().\n", prog) ;
      exit(1) ;
    }
  }

  while(1)
  {
    char name[MAX_LINE_LEN] ;

    if(fread((void *)&pe, sizeof pe, (size_t)1, stdin) < 1)
    {
      if(feof(stdin))
      {
        if(ignore_header)
        {
          exit(0) ;
        }
        else
        {
          if(n == h.no_recs)
          {
            exit(0) ;
          }
          else
          {
            fflush(stdout) ;
            fprintf(stderr, "%s: expected %ld records, but actually saw %ld.\n", prog,
                    (long)h.no_recs, n) ;
            exit(1) ;
          }
        }
      }
      else
      {
        fflush(stdout) ;
        fprintf(stderr, "%s: error fread'ing parser entry.\n", prog) ;
        exit(1) ;
      }
    }
    len = (pe.slen + 3) & ~0x03 ;
    if(fread((void *)name, (size_t)len, (size_t)1, stdin) < 1)
    {
      if(feof(stdin))
      {
        fflush(stdout) ;
        fprintf(stderr, "%s: unexpected eof while trying to read file name (rec #%ld).\n",
                prog, n) ;
        exit(1) ;
      }
      else
      {
        fflush(stdout) ;
        fprintf(stderr, "%s: error fread'ing %ld bytes for file name (rec #%ld).\n",
                prog, (long)len, n) ;
        exit(1) ;
      }
    }
    *(name + len) = '\0' ;
    if( ! print_parser_info(stdout, &pe, name, n))
    {
      fflush(stdout) ;
      fprintf(stderr, "%s: error from print_parser_info() (rec #%ld).\n", prog, n) ;
      exit(1) ;
    }
    n++ ;
  }
  return 0 ;                    /* for gcc -Wall */
}


int
print_parser_info(fp, pe, name, n)
  FILE *fp ;
  parser_entry_t *pe ;
  char *name ;
  int n ;
{
  char out[MAX_LINE_LEN] ;
  char *s = out ;

  ptr_check(fp, FILE, "print_parser_info", 0) ;
  ptr_check(pe, parser_entry_t, "print_parser_info", 0) ;
  ptr_check(name, char, "print_parser_info", 0) ;

  if(CSE_IS_DIR(pe->core))
  {
    sprintf(out, "%5d %5d %5d\t", n, pe->core.parent_idx, pe->core.child_idx) ;
  }
  else
  {
    sprintf(out, "%5d %5d %5s\t", n, pe->core.parent_idx, " ") ;
  }

  strcat(out, name) ; strcat(out, " ") ;
  db2v_size(pe, s = strend(s)) ; strcat(out, " ") ;
  db2v_date(pe, s = strend(s), 64) ; strcat(out, " ") ;
  db2v_owner(pe, s = strend(s)) ; strcat(out, " ") ;
  db2v_perms(pe, s = strend(s)) ;

  return puts(out) != EOF ;
}


void
usage()
{
  fprintf(stderr, "Usage: %s [-h]\n", prog) ;
  exit(1) ;
}
