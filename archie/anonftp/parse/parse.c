/*
  Usage:
    <some>_parser [-h] [-i <input-file>] [-o <output-file>] [-p <prep-dir>] [-r <root-dir>]

  Read a recursive directory listing and write it out in a form suitable for insertion into
  the archie database.

  Definitions:

    a "directory declaration" (DDec) is the listing of a directory in the same manner as a
    listing of an ordinary file (i.e. permissions, owner, date modified, etc.).

    a "directory definition" (DDef) is the listing of the contents of the directory.

    a directory definition header (DDH) is a line specifying to which directory the
    following DDef belongs.


  1) the parser would like all DDHs to be rooted (any root will do).

  2) most (all?) UNIX listings don't start with a DDH.

  Therefore, if all but the first (missing) DDHs are rooted (usually ./) then use the -r
  option to supply the first DDH (e.g. -r .:).  If, as well, the other DDHs are not rooted
  then use the -p option to prepend a root to them (e.g. -p .).
    
                                                         - Bill Heelan (wheelan@cs.mcgill.ca)

  -----------------------------------------------------------------------------------------

  while( ! eof)
    read line
      if starts directory list then
        while top of stack queue is empty
          pop the stack
          remove entry from queue
          push it on the stack
          if ! (path through stack is same as current path)
            abort
      else
        if is a directory file then
          add entry to queue
        create new core record
        store core record
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defines.h"
#include "parse.h"
#include "utils.h"
#include "stack.h"
#include "queue.h"
#include "input.h"
#include "line_type.h"
#include "pars_ent_holder.h"
#include "storage.h"
#include "output.h"
#include "header.h"
#include "typedef.h"
#include "error.h"
#include "lang_parsers.h"
#ifdef SOLARIS
#include "protos.h"
#else

#ifdef __STDC__

extern int check_stack(void) ;
extern int handle_dir_start(char *line, char *prep_dir, int state) ;
extern int handle_file(char *line, int state) ;
extern int output_header(FILE *ofp, int ignore_header, header_t *hrec, int parse_failed) ;
extern int parser_output(FILE *ofp, header_t *hrec) ;
extern void usage(void) ;

#else

extern int check_stack(/* void */) ;
extern int handle_dir_start(/* char *line, char *prep_dir, int state */) ;
extern int handle_file(/* char *line, int state */) ;
extern int output_header(/* FILE *ofp, int ignore_header, header_t *hrec, int parse_failed */) ;
extern int parser_output(/* FILE *ofp, header_t *hrec */) ;
extern void usage(/* void */) ;

#endif
#endif

char *prog ;
int verbose = 0 ;

#define MAX_TABLE_SIZE 20
#define MAX_ERROR_NUM 20

int local_timezone = 0;
static int dir_seen = 0;
static int error_seen = 0;
static int files_seen = 0;
static int blank_seen = 0;
static int retry_mode  = 0;

static char *table[MAX_TABLE_SIZE];
static int table_num;


void
usage()
{
  error(A_ERR, "usage", USAGE_001, prog);
  exit(1) ;
}


static char *build_dir(prep_dir,line)
char *prep_dir,*line;
{

   char *t,*tmp,*tmp_dir;
   int len;


   t = strchr(line,':' );
   if ( t == NULL )
      return NULL;

   tmp = t;
   while ( t != line && *t != '/' ) 
      t--;

   if ( t == line ) 
      t = tmp;


     len = strlen(prep_dir)+1+(t-line)+1+1;
     tmp_dir = (char*)malloc(sizeof(char)*len );
     if ( tmp_dir == NULL )
       return NULL;

     strcpy(tmp_dir,prep_dir);
     strcat(tmp_dir,"/");
     strncat(tmp_dir,line,t-line);
     tmp_dir[len-2] = ':';
     tmp_dir[len-1] = '\0';
#if 0
   if ( strncmp(line,"./",2) == 0 ) {
     }
   else {
     len = (t-line)+1+1;
     tmp_dir = (char*)malloc(sizeof(char)*len );
     if ( tmp_dir == NULL )
       return NULL;
     strncpy(tmp_dir,line,t-line);
     tmp_dir[len-2] = ':';
     tmp_dir[len-1] = '\0';
   }
#endif

   return tmp_dir;
}



int
main(ac, av)
  int ac ;
  char **av ;
{
  char *infile = (char *)0 ;
  char line[2 * MAX_LINE_LEN + 1] ;
  char origline[2 * MAX_LINE_LEN + 1] ;
  char *lp ;
  char *outfile = (char *)0 ;
  char *prep_dir = (char *)0 ;
  char *root_dir = (char *)0 ;
  FILE *infp = stdin ;
  FILE *outfp = stdout ;
  header_t hrec ;
  int done = 0 ;
  int parse_error = 0 ;
  int ignore_header = 0 ;
  int old_state ;
  int state;
  int logging = 0;
  pathname_t logfile;
  pathname_t orig_root_dir;
  pathname_t tmp_root_dir;

  orig_root_dir[0] = tmp_root_dir[0] = '\0';
  
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
        ignore_header = 1 ;     /* the listing is not expected to have a header */
        break ;

      case 'i':
        if( ! (av++, --ac))
        {
          usage() ;
        }
        else
        {
          infile = av[0] ;
        }
        break ;

      case 'o':
        if( ! (av++, --ac))
        {
          usage() ;
        }
        else
        {
          outfile = av[0] ;
        }
        break ;

      case 'p':
        if( ! (av++, --ac))
        {
          usage() ;
        }
        else
        {
          prep_dir = av[0] ;
        }
        break ;

      case 'r':
        if( ! (av++, --ac))
        {
          usage() ;
        }
        else
        {
          root_dir = av[0] ;
        }
        break ;

      case 'L':
        if( ! (av++, --ac))
        {
          usage() ;
        }
        else
        {
          strcpy(logfile, av[0]);
          logging = 1;
        }
        break ;

      case 'l':
        logging = 1;
        break;

      case 'v':
        verbose = 1 ;
        break ;

      default:
        usage() ;
        break ;
      }
    }
  }

  /* set up logs */
   
  if(logging)
  {  
    if(logfile[0] == '\0')
    {
      if(open_alog((char *) NULL, A_INFO, prog) == ERROR)
      {

        /*  "Can't open default log file" */

        error(A_ERR, "parse_anonftp", PARSE_ANONFTP_017);
        exit(ERROR);
      }
    }
    else
    {
      if(open_alog(logfile, A_INFO, prog) == ERROR)
      {

        /* "Can't open log file %s" */

        error(A_ERR, "parse_anonftp", PARSE_ANONFTP_018, logfile);
        exit(ERROR);
      }
    }
  }
  if ( root_dir != NULL ) {
    strcpy(tmp_root_dir, root_dir);
    strcpy(orig_root_dir ,root_dir);
  }

 retry_with_blanks:
  if ( blank_seen && root_dir != NULL ) {
    strcpy(tmp_root_dir,orig_root_dir);
    root_dir = tmp_root_dir;
  }
    
retry_with_new_root:

  if ( retry_mode ) {
    free_elts();
  }

  state = L_BLANK ;

  if(infile != (char *)0)
  {
    if(strcmp(infile, "-") != 0)
    {
      if((infp = fopen(infile, "r")) == (FILE *)0)
      {

        /* "Can't open file %s for reading" */

        error(A_ERR, "parse_anonftp", PARSE_ANONFTP_001, infile) ;
        exit(ERROR) ;
      }
    }
  }
  if(outfile != (char *)0)
  {
    if(strcmp(outfile, "-") != 0)
    {
      if((outfp = fopen(outfile, "w")) == (FILE *)0)
      {

        /* "can't open %s for writing" */

        error(A_SYSERR,"parse_anonftp", PARSE_ANONFTP_002, outfile) ;
        exit(1) ;
      }
    }
  }

  if( ! ignore_header && read_header(infp, &hrec, (u32 *)0, 0,0) != A_OK)
  {
    error(A_ERR,"Error from read_header() file %s", infile);
    exit(ERROR) ;
  }

  if(HDR_GET_TIMEZONE(hrec.header_flags))
  {
    local_timezone = hrec.timezone;
  }

  if( ! S_init_parser())
  {
    /* "Can't initialize line parser" */
  
    error(A_ERR, "parse_anonftp", PARSE_ANONFTP_004);
    exit(ERROR) ;
  }
  if( /*! retry_mode &&*/ ! set_elt_size(sizeof(Pars_ent_holder)))
  {
    error(A_ERR, "parse_anonftp", PARSE_ANONFTP_005);
    exit(ERROR) ;
  }
  if(root_dir != (char *)0)
  {
    if( ! init_stack(root_dir))
    {

      /* "Can't initialize directory stack" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_006);
      exit(ERROR) ;
    }
  }
  else
  {
    parse_error = 0;
    while( ! done)              /* initialize a root directory */
    {
      if( ! get_line(line, MAX_LINE_LEN, infp))
      {
        done = 1 ;
        parse_error = 1 ;
        strcpy(origline, line);
	
        /* "Error trying to initialize root directory" */

        error(A_ERR, "parse_anonftp", PARSE_ANONFTP_007);
      }
      else
      {
        strcpy(origline, line);

        old_state = state ;
        switch(state = S_line_type(line))
        {

          /* "Unexpected %s line #%d in listing" */

#define badline(ltype) \
          do { error(A_WARN, "parse_anonftp", PARSE_ANONFTP_008, ltype, line_num()) ; parse_error++;  if ( parse_error >5) done = 1; } while(0)

        case L_BLANK:
          break ;
        case L_UNKNOWN:
          badline("unknown") ;
          break ;
        case L_ERROR:
          badline("error") ;
          break ;
        case L_TOTAL:
          badline("total") ;
          break ;
        case L_CONT:
          badline("continuation") ;
          break ;
        case L_PARTIAL:
          badline("partial") ;
          break ;
        case L_FILE:
          badline("file") ;
          break ;
        case L_UNREAD:
          badline("unreadable") ;
          break ;

        case L_DIR_START:
          if( ! init_stack(line))
          {

            /* "Error initializing directory stack with root directory" */

            error(A_ERR, "parse_anonftp", PARSE_ANONFTP_009);
            parse_error = 1 ;
          }
          done = 1 ;
          break ;

        default:
          error(A_ERR, "parse_anonftp", PARSE_ANONFTP_010,line_num()) ;
          parse_error = 1 ;
          done = 1 ;
          break ;
        }
      }
    }
  }
  if( ! parse_error)
  {
    done = 0 ;
    lp = line ;
    if ( ! process_fake_entries() ) {
      parse_error = done = 1;
    }
    while( ! done)
    {
      if( ! get_line(lp, MAX_LINE_LEN, infp))
      {
        mck ;
        done = 1 ;
      }
      else
      {
        mck ;
        old_state = state ;
        strcpy(origline, line);	
        switch(state = S_line_type(lp))
        {
        case L_BLANK:
        case L_ERROR:
        case L_TOTAL:
        case L_UNREAD:          /* will later get flagged as missing */
          break ;

        case L_CONT:
          if(old_state == L_PARTIAL)
          {
            skip_line() ;
          }
          else
          {
            badline("continuation") ;
          }
          break ;

        case L_PARTIAL:
          lp = strend(lp) ; *lp = ' ' ; *++lp = '\0' ;
          break ;

        case L_UNKNOWN:
          badline("unknown") ;
          break ;

        case L_FILE:
          mck ;
          files_seen++;
          if( ! handle_file(line, state))
          {
            blank_seen++;
            error_seen++;
            if ( error_seen > MAX_ERROR_NUM ) {
              error(A_ERR,"parse_anonftp", "Too many errors .. aborting");
              parse_error = done = 1;
            }
            if ( infp != stdin && outfp != stdout) {
              fclose(infp);
              fclose(outfp);
              retry_mode = 1;
              init_line_num();
              goto retry_with_blanks;
            }
            else {
              /* "Cannot figure out the number of leading blanks " */
              error(A_ERR,"parse_anonftp", "Cannot figure out the number of leading blanks") ;

              /* "Error from handle_dir_start() on line #%d: aborting" */

              error(A_ERR,"parse_anonftp", PARSE_ANONFTP_012, line_num()) ;
              parse_error = done = 1 ;
            }
                
          }
        
          mck ;
          break ;

        case L_DIR_START:

          mck ;
          dir_seen++;
          if( ! handle_dir_start(line, prep_dir, state))
          {
            error_seen++;
            if ( error_seen > MAX_ERROR_NUM ) {

              error(A_ERR,"parse_anonftp", "Too many errors...aborting");
              parse_error = done = 1 ;
              break;
            }
            else 
              {
              int num;
              if ( (num = strspn(origline," ")) ){
                set_blanks(num);
                  fclose(infp);
                  fclose(outfp);
                  retry_mode = 1;
                  init_line_num();
                blank_seen = 1;
                goto retry_with_blanks;
              }
              else
              if ( infp != stdin && outfp != stdout) {

                /* if one does ls -lR pub .. one will not see
                   the name of the directory pub listed
                   as    pub:
                */
                if ( files_seen && table_num == 0 ) {

                  root_dir = build_dir(prep_dir,origline);                  

                  if ( root_dir != NULL ) {
                    /* "Trying to figure out the root_dir %s" */
                    error(A_INFO,"parse_anonftp", PARSE_ANONFTP_019, root_dir);
                    fclose(infp);
                    fclose(outfp);
                    retry_mode = 1;
                    init_line_num();
                    strcpy(tmp_root_dir, root_dir);
                    root_dir = tmp_root_dir;
                    goto retry_with_new_root;
                  }
                  else {
                    /* "Cannot determine the root_dir" */
                    error(A_INFO,"parse_anonftp", PARSE_ANONFTP_021);

                    /* "Error from handle_dir_start() on line #%d: aborting" */

                    error(A_ERR,"parse_anonftp", PARSE_ANONFTP_012, line_num()) ;
                    parse_error = done = 1 ;
                  }
                }
                else {
                  
                  /* Probably the user did ls-lR pub1 pub2 .. pubn
                     In which case the name of the directory appears */
                  
                  if ( ! add_fake_entry(line)  ) {
                    error(A_ERR,"handle_dir_start","Too many errors, aborting");
                    parse_error = done = 1 ;
                  }
                  else {
                    fclose(infp);
                    fclose(outfp);
                    retry_mode = 1;
                    init_line_num();
                    strcpy(tmp_root_dir, orig_root_dir);
                    root_dir = tmp_root_dir;
                    goto retry_with_new_root;
                  }

                }
              }
              else {
                /* "Cannot figure out root_dir due to usage of standard streams" */
                error(A_ERR,"parse_anonftp", PARSE_ANONFTP_020) ;

                /* "Error from handle_dir_start() on line #%d: aborting" */

                error(A_ERR,"parse_anonftp", PARSE_ANONFTP_012, line_num()) ;
                parse_error = done = 1 ;
              }

            }
          }
          mck ;
#ifdef DEBUG
          error(A_INFO, "parse_anonftp", "Post-dir-start") ; print_stack() ;
#endif
          break ;

        default:

          /* "Unknown state = '%d' on line #%d" */

          error(A_ERR, "parse_anonftp", PARSE_ANONFTP_013, state, line_num()) ;
          exit(1) ;
          break ;
        }
        if(state != L_PARTIAL)
        {
          lp = line ;
        }
      }
    }
  }
  mck ;

  if(parse_error)
  {

    /* "Error at line #%d: '%s'" */

    error(A_ERR, "parse_anonftp", PARSE_ANONFTP_014, line_num(), origline);

    sprintf(hrec.comment, PARSE_ANONFTP_014, line_num(), origline) ;
    HDR_SET_HCOMMENT(hrec.header_flags);

    hrec.format = FRAW;
    HDR_SET_FORMAT(hrec.header_flags);

    if( ! output_header(outfp, ignore_header, &hrec, parse_error))
    {

      /* Error from output_header(): aborting" */

      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_015);
    }
    exit(1) ;
  }
  else
  {
    if(check_stack())
    {
      /* parse_error if no records were processed */
      
      if( ! output_header(outfp, ignore_header, &hrec, elt_num() < 1))
      {
        /* "Error from output_header(): aborting" */
        error(A_ERR, "parse_anonftp", PARSE_ANONFTP_015);
        exit(1) ;
      }
      else
      {
        parser_output(outfp, &hrec) ;
        exit(0) ;
      }
    }
    else
    {

      /* "Error from check_stack() on line #%d: aborting" */
    
      error(A_ERR, "parse_anonftp", PARSE_ANONFTP_016, line_num()) ;
      if( ! output_header(outfp, ignore_header, &hrec, 1))
      {
        /* "Error from output_header(): aborting" */
        error(A_ERR,"parse_anonftp", PARSE_ANONFTP_015);
      }
      exit(1) ;
    }
  }
  exit(A_OK);
  return(A_OK);                 /* keep gcc -Wall happy */
}


/*
  If there are any directories left on the stack (i.e. there is a non-empty queue) then
  there is a directory that was listed as a file, but has no corresponding listing.
    
  Only print out warning messages.
*/

int
check_stack()
{
  while( ! empty_stack())
  {
    if( ! pop())
    {
      while( ! empty_queue(tos()))
      {
        Queue_elt *qe ;

	/* "Expected listing for the directory '%s' at the end" */

#ifdef DEBUG
	error(A_ERR, "check_stack", CHECK_STACK_001, tos()->q.head->ident->name) ;
#endif
	if((qe = remq(tos())) == (Queue_elt *)0)
        {

	  /* "Error removing queue element from top of stack" */

	  error(A_ERR,"check_stack", CHECK_STACK_002);
          return 0 ;
        }
        dispose_Queue_elt(qe) ;
      }
    }
  }
  return 1 ;
}


/*
  Check that the sub-directory listing is the one that is expected (i.e. it is at the head
  of the top-most non-empty queue on the stack.
*/

int
handle_dir_start(line, prep_dir, state)
  char *line ;
  char *prep_dir ;
  int state ;
{
  char **p ;
  int dev ;
  int n ;
  Queue_elt *qe ;

  ptr_check(line, char, "handle_dir_start", 0) ;

  if( ! S_split_dir(line, prep_dir, &dev, &n, &p))
  {

    /* "Error from S_split_dir()" */
  
    error(A_ERR, "handle_dir_start", HANDLE_DIR_START_001); 
    return 0 ;
  }

  /*
    If it turns out the directory we are expecting to see at this point isn't here, then
    drop it and look for the next in the sequence (by jumping to try_next_dir).
  */

 try_next_dir:

  while(empty_queue(tos()))
  {
    if( ! pop())
    {

      /* "Error from pop() while looking for non-empty queue" */

      error(A_ERR, "handle_dir_start", HANDLE_DIR_START_002);
      return 0 ;
    }
  }
  if(empty_stack())
  {
    pathname_t new_line;
    
#ifdef __STDC__      
    extern int vfprintf_path(FILE *fp, char **p, int n);
#else
    extern int vfprintf_path();
#endif

    /* "directory '" */

    error(A_ERR, "handle_dir_start", HANDLE_DIR_START_003);
    vfprintf_path(stderr, p, n) ;

    /* "' was not previously declared" */

    fprintf(stderr, HANDLE_DIR_START_004);
    return 0 ;
    
  }
  if((qe = remq(tos())) == (Queue_elt *)0)
  {

    /* "Error removing queue element from top of stack" */

    error(A_ERR, "handle_dir_start", HANDLE_DIR_START_005);
    return 0 ;
  }
  if( ! push(qe->ident))
  {

    /* "Error from push()" */

    error(A_ERR, "handle_dir_start", HANDLE_DIR_START_006);
    return 0 ;
  }
  qe->ident = (Info_cell *)0 ;
  dispose_Queue_elt(qe) ;
  if( ! stack_match((const char **)p, n))
  {

    /* "Error expected listing for directory %s on line %d" */

#ifdef DEBUG
    error(A_ERR, "handle_dir_start", HANDLE_DIR_START_007, tos()->ident->name, line_num()) ;
#endif

    pop() ;
    goto try_next_dir ;         /* I really ought to get rid of this... */
  }
  tos()->ident->addr->pent.core.child_idx = elt_num() + 1 ;
  return 1 ;
}


/*
  The input string will be overwritten if S_file_parse() is called.
*/

int
handle_file(line, state)
  char *line ;
  int state ;
{
  Pars_ent_holder *peh ;
  char *name ;
  int is_dir ;

  ptr_check(line, char, "handle_file", 0) ;

  if((peh = (Pars_ent_holder *)new_elt()) == (Pars_ent_holder *)0)
  {

    /* "Error allocating space for parser record" */

    error(A_ERR, "handle_file", HANDLE_FILE_001);
    return 0 ;
  }
  if((name = S_file_parse(line, &peh->pent, &is_dir)) == (char *)0)
  {

    /* "Error from S_file_parse()" */

    error(A_ERR, "handle_file", HANDLE_FILE_002, line_num());
    return 0 ;
  }
  if( ! is_dir)
  {
    CSE_SET_NON_DIR(peh->pent.core) ;
  }
  else                          /* directory file */
  {
    char *s ;
    Queue_elt *qe ;

    if((s = S_dup_dir_name(name)) == (char *)0)
    {

      /* "Error strdup'ing directory name" */

      error(A_ERR, "handle_file", HANDLE_FILE_003);
      return 0 ;
    }
    CSE_SET_DIR(peh->pent.core) ;
    qe = new_Queue_elt(s) ;
    qe->ident->addr = peh ;
    qe->ident->idx = elt_num() ;
    if( ! addq(qe, tos()))
    {

      /* "Error adding directory to queue" */

      error(A_ERR, "handle_file", HANDLE_FILE_004);
      return 0 ;
    }
  }
  peh->pent.core.parent_idx = tos()->ident->idx ;
  if((peh->name = strdup(name)) == (char *)0)
  {

    /* "Error strdup'ing file name" */

    error(A_ERR, "handle_file", HANDLE_FILE_005);
    return 0 ;
  }
  return 1 ;
}


int
output_header(ofp, ignore_header, hrec, parse_failed)
  FILE *ofp ;
  int ignore_header ;
  header_t *hrec ;
  int parse_failed ;
{
  ptr_check(ofp, FILE, "output_header", 0) ;
  ptr_check(hrec, header_t, "output_header", 0) ;

  if( ! ignore_header)
  {
    hrec->generated_by = PARSER ;
    HDR_SET_GENERATED_BY(hrec->header_flags) ;

    hrec->no_recs = elt_num() + 1 ; /* starts at 0 */
    HDR_SET_NO_RECS(hrec->header_flags) ;

    hrec->update_status = parse_failed ? FAIL : SUCCEED ;
    HDR_SET_UPDATE_STATUS(hrec->header_flags) ;

    hrec->parse_time = (date_time_t)time((time_t *)0) ;
    HDR_SET_PARSE_TIME(hrec->header_flags) ;

    if(write_header(ofp, hrec, (u32 *)0, 0, 0) != A_OK)
    {

      /* "Error from write_header()" */

      error(A_ERR, "parser_output", HANDLE_FILE_006);
      return 0 ;
    }
  }
  return 1 ;
}


/*
  Write the processed listing to the output file.
*/

int
parser_output(ofp, hrec)
  FILE *ofp ;
  header_t *hrec ;
{
  Pars_ent_holder *peh ;

  ptr_check(ofp, FILE, "parser_output", 0) ;
  ptr_check(hrec, header_t, "parser_output", 0) ;

  if((peh = (Pars_ent_holder *)first_elt()) == (Pars_ent_holder *)0)
  {
    /* "Error from first_elt()" */
  
    error(A_ERR, "parser_output", PARSER_OUTPUT_001);
    return 0 ;
  }
  do
  {
    if( ! print_core_info(ofp, &peh->pent, peh->name))
    {

      /* "Error from print_core_info()" */

      error(A_ERR, "parser_output", PARSER_OUTPUT_002);
      return 0 ;
    }
  }
  while((peh = (Pars_ent_holder *)next_elt()) != (Pars_ent_holder *)0) ;
  free_elts() ;
  return 1 ;
}


static int add_fake_entry(line)
  pathname_t line;
{

  if ( table_num == MAX_TABLE_SIZE ) {
    return 0;
  }

  table[table_num] = strdup(line);
  table_num++;
  return 1;
}


static int process_fake_entries()
{
  int i;
  char fake_line[100];

  for ( i = 0; i < table_num; i++ ) {
    strcpy(fake_line,
           "drwxr-xr-x  2 root          512 Jan  1  1971 ");
    strcat(fake_line,table[i]);
    if ( ! handle_file(fake_line,0) ) {
      return 0;
    }
  }

  return 1;

}
