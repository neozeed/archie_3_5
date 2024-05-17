#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "protos.h"
#include "parse.h"
#include "parser_file.h"
#include "output.h"
#include "lang_parsers.h"
#include "error.h"

/*
  Write a parser entry, followd by the file name to the output file.  The
  file name is padded (with Satanic messages, when viewed backwards) to a
  multiple of 4 bytes.  The file name (when written) is not nul
  terminated; the length field should reflect this.
*/

int
print_core_info(fp, pe,name)
  FILE *fp ;
  parser_entry_t *pe ;
  char *name ;
{
  char padding[PAD - 1] ;
  int npad ;

  ptr_check(fp, FILE, "print_core_info", 0) ;
  ptr_check(pe, parser_entry_t, "print_core_info", 0) ;
  ptr_check(name, char, "print_core_info", 0) ;

  memset(padding, '\0', sizeof padding) ;
  if(fwrite((void *)pe, sizeof *pe, (size_t)1, fp) != 1)
  {

    /* "Error fwrite'ing parser_entry_t structure"  */
  
    error(A_ERR, "print_core_info", PRINT_CORE_INFO_001);
    return 0 ;
  }
  if(fwrite((void *)name, (size_t)pe->slen, (size_t)1, fp) != 1)
  {

    /* "Error fwrite'ing file name" */
  
    error(A_ERR, "print_core_info", PRINT_CORE_INFO_002);
    return 0 ;
  }
  if((npad = pe->slen % PAD) != 0)
  {
    if(fwrite((void *)padding, (size_t)(PAD - npad), (size_t)1, fp) != 1)
    {

      /* "Error fwrite'ing padding" */
    
      error(A_ERR,"print_core_info", PRINT_CORE_INFO_003);
      return 0 ;
    }
  }
  return 1 ;
}
