#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>

#define _toupper(c)	((c)-'a'+'A')

#ifdef MMAP
#include <sys/mman.h>
#endif

/* Archie definitions */
#include <defines.h>
#include <error.h>

#include "prarch.h"

#include <pfs.h>
#include <perrno.h>
#include <plog.h>

VLINK	atoplink();

char *re_comp();
char *make_lcase();
int get_match_list();

extern char *strings_begin;
extern long strings_table_size;
extern DBM *fast_strings;

/* So we can adjust our cache policy based on queue length */
extern int  pQlen;

static	char	lowertable[256] = { 
'\000','\001','\002','\003','\004','\005','\006','\007',
'\010','\011','\012','\013','\014','\015','\016','\017',
'\020','\021','\022','\023','\024','\025','\026','\027',
'\030','\031','\032','\033','\034','\035','\036','\037',
' ','!','"','#','$','%','&','\'',
'(',')','*','+',',','-','.','/',
'0','1','2','3','4','5','6','7',
'8','9',':',';','<','=','>','?',
'@','a','b','c','d','e','f','g',
'h','i','j','k','l','m','n','o',
'p','q','r','s','t','u','v','w',
'x','y','z','[','\\',']','^','_',
'`','a','b','c','d','e','f','g',
'h','i','j','k','l','m','n','o',
'p','q','r','s','t','u','v','w',
'x','y','z','{','|','}','~','\177',
'\200','\201','\202','\203','\204','\205','\206','\207',
'\210','\211','\212','\213','\214','\215','\216','\217',
'\220','\221','\222','\223','\224','\225','\226','\227',
'\230','\231','\232','\233','\234','\235','\236','\237',
'\240','\241','\242','\243','\244','\245','\246','\247',
'\250','\251','\252','\253','\254','\255','\256','\257',
'\260','\261','\262','\263','\264','\265','\266','\267',
'\270','\271','\272','\273','\274','\275','\276','\277',
'\300','\301','\302','\303','\304','\305','\306','\307',
'\310','\311','\312','\313','\314','\315','\316','\317',
'\320','\321','\322','\323','\324','\325','\326','\327',
'\330','\331','\332','\333','\334','\335','\336','\337',
'\340','\341','\342','\343','\344','\345','\346','\347',
'\350','\351','\352','\353','\354','\355','\356','\357',
'\360','\361','\362','\363','\364','\365','\366','\367',
'\370','\371','\372','\373','\374','\375','\376','\377'};

#define MATCH_CACHE_SIZE     15

struct match_cache {
    char                *arg;	     /* Matched regular expression          */
    int			max_hits;    /* Maximum matchess <0 = found all     */
    int			offset;      /* Offset                              */
    search_sel 		search_type; /* Search method (the one used)        */
    search_sel          req_type;    /* Requested method                    */
    VLINK		matches;     /* Matches                             */
    VLINK		more;	     /* Additional matches                  */
    int			archiedir;   /* Flag: directory links are to archie */
    struct match_cache 	*next;       /* Next entry in cache                 */
};

static struct match_cache *mcache = NULL;

static int		  cachecount = 0;

/*
 * prarch_match - Search archie database for specified file
 *
 * 	PRARCH_MATCH searches the archie database and returns
 *      a list of files matching the provided regular expression
 *      
 *  ARGS:  program_name - regular expression for files to match
 *             max_hits - maximum number of entries to return (max hits)
 *               offset - start the search after this many hits
 *          search_type - search method 
 *                   vd - pointer to directory to be filled in
 *            archiedir - flag - directory links should be to archie
 *
 *   Search method is one of:   S_FULL_REGEX
 *		                S_EXACT 
 *                              S_SUB_NCASE_STR 
 *                              S_SUB_CASE_STR 
 */

int prarch_match(program_name,max_hits,offset,search_type,vd,archiedir)
    char	*program_name; 	/* Regular expression to be matched       */
    int		max_hits;	/* Maximum number of entries to return    */
    int		offset;		/* Skip this many matches before starting */
    search_sel search_type;	/* Search method                          */
    VDIR	vd;		/* Directory to be filled in              */
    int		archiedir;	/* Flag: directory links s/b to archie    */
{
  /*
   * Search the database for the string specified by 'program_name'.  Use the
   * fast dbm strings database if 'is_exact' is set, otherwise search through
   * the strings table.  Stop searching after all matches have been found, or
   * 'max_hits' matches have been found, whichever comes first.  
   */
  char 		s_string[MAX_STRING_LEN];
  char		*strings_ptr;
  char		*strings_curr_off;
  strings_header str_head;
  datum 	search_key, key_value;
  search_sel 	new_search_type = S_EXACT;    /* Alternate search method */
  search_sel 	or_search_type = search_type; /* Original search method */
  int 		nocase = 0;
  int 		hits_exceeded = FALSE;	      /* should be boolean? */
  char 		*strings_end;
  int 		match_number;
  int 		patlen;
  site_out 	**site_outptr;
  site_out 	site_outrec;
  int 		i;
  VLINK		cur_link;
  int		loopcount = 0;
  int		retval;

  if(!program_name || !(*program_name)) return(PRARCH_BAD_ARG);

  strcpy(s_string, program_name);

  /* See if we can use a less expensive search method */
  if((search_type == S_FULL_REGEX) && 
     (strpbrk(s_string,"\\^$.,[]<>*+?|(){}/") == NULL)) 
      or_search_type = search_type = S_SUB_CASE_STR;
  else if((search_type == S_E_FULL_REGEX) && 
	  (strpbrk(s_string,"\\^$.,[]<>*+?|(){}/") == NULL))
      or_search_type = search_type = S_E_SUB_CASE_STR;

  /* The caching code assumes we are handed an empty directory */
  /* if not, return an error for now.  Eventually we will get  */
  /* rid of that assumption                                    */
  if(vd->links) {
      plog(L_DIR_ERR, NULL, NULL, "Prarch_match handed non empty dir",0);
      return(PRARCH_BAD_ARG);
  }

  if(check_cache(s_string,max_hits,offset,search_type,
		 archiedir,&(vd->links)) == TRUE) {
      plog(L_DB_INFO, NULL, NULL, "Responding with cached data",0);
      return(PSUCCESS);
  }

  site_outptr = (site_out **) malloc((unsigned)(sizeof(site_out) * 
						(max_hits + offset)));
  if(!site_outptr) return(PRARCH_OUT_OF_MEMORY);

 startsearch:

  strings_ptr = strings_begin;
  strings_end = strings_begin + (int) strings_table_size;

  match_number = 0;

  switch(search_type){

  case S_E_SUB_CASE_STR:
      new_search_type = S_SUB_CASE_STR;
      goto exact_match;
  case S_E_SUB_NCASE_STR:
      new_search_type = S_SUB_NCASE_STR;
      goto exact_match;
  case S_E_FULL_REGEX:
      new_search_type = S_FULL_REGEX;
  exact_match:
  case S_EXACT:

      search_key.dptr = s_string;
      search_key.dsize = strlen(s_string) + 1;

      check_for_messages();
      key_value = dbm_fetch(fast_strings, search_key) ;

      if(key_value.dptr != (char *)NULL){ /* string in table */

	int string_pos;

	bcopy(key_value.dptr,(char *)&string_pos, key_value.dsize);

	strings_ptr += string_pos;

	bcopy(strings_ptr,(char *)&str_head,sizeof(strings_header));

	check_for_messages();

	if(str_head.filet_index != -1) {
	    retval = get_match_list((int) str_head.filet_index, max_hits + 
				    offset, &match_number, site_outptr, FALSE);
	    
	    if((retval != A_OK) && (retval != HITS_EXCEEDED)) {
	      plog(L_DB_ERROR,NULL,NULL,"get_match_list failed (%d)",retval,0);
	      goto cleanup;
	    }

	    if( match_number >= max_hits + offset ){
		hits_exceeded = TRUE;
		break;
	    }
	}
      }
      else if (search_type != S_EXACT) { /* Not found - but try other method */
	  search_type = new_search_type;
	  goto startsearch;
      }
      break;

  case S_FULL_REGEX:
	
      if(re_comp(s_string) != (char *)NULL){
	  return (PRARCH_BAD_REGEX);
      }

      str_head.str_len = -1;

      check_for_messages();

      while((strings_curr_off = strings_ptr + str_head.str_len + 1) < strings_end){

	if((loopcount++ & 0x7ff) == 0) check_for_messages();

	strings_ptr = strings_curr_off;

	bcopy(strings_ptr,(char *)&str_head,sizeof(strings_header));

	strings_ptr += sizeof(strings_header);
	    
	if(re_exec( strings_ptr ) == 1 ){ /* TRUE */
	  strings_curr_off = strings_ptr;

	  check_for_messages();

	  if(str_head.filet_index != -1){
	    retval = get_match_list((int) str_head.filet_index, max_hits + 
				   offset, &match_number, site_outptr, FALSE);

	    if((retval != A_OK) && (retval != HITS_EXCEEDED)) {
	      plog(L_DB_ERROR,NULL,NULL,"get_match_list failed (%d)",retval,0);
	      goto cleanup;
	    }

	    if( match_number >= max_hits + offset ){
	      hits_exceeded = TRUE;
	      break;
	    }
	  }
        }
      }

      break;

#define TABLESIZE 256

  case S_SUB_NCASE_STR:
      nocase++;
  case S_SUB_CASE_STR: 	  {
      char			pattern[MAX_STRING_LEN];
      int			skiptab[TABLESIZE];
      register int		pc, tc;
      register int		local_loopcount = 0xfff;
      char			*bp1;
      int			skip;
      int			plen;
      int			plen_1;
      int			tlen;
      unsigned char		tchar; 

      plen = strlen(s_string);
      plen_1 = plen -1;

      /* Old code (replaced by inline code taken from initskip)       */
      /* patlen = strlen(s_string ) ;                                 */
      /* initskip(s_string, patlen, search_type == S_SUB_NCASE_STR) ; */

      if(nocase) {
	  for(pc = 0; s_string[pc]; pc++)
	      pattern[pc] = lowertable[s_string[pc]];
	  pattern[pc] = '\0';
      }
      else strcpy(pattern,s_string);

      for( i = 0 ; i < TABLESIZE ; i++ ) 
	  skiptab[ i ] = plen;

      /* Note that we want both ucase and lcase in this table if nocase */
      for( i = 0, tchar = *pattern; i < plen ; i++, tchar = *(pattern + i)) {
	  skiptab[tchar] = plen - 1 - i;
	  if(nocase && islower(tchar)) 
	      skiptab[_toupper(tchar)] = plen - 1 - i;
      }
      
      /* Begin heavily optimized and non portable code */

      /* Note that we are depending on str_head being 8 bytes */
      tlen = -9;                          /* str_head.str_len */

      strings_curr_off = strings_ptr;

      while((strings_curr_off += tlen + 9) < strings_end) {
	  if(--local_loopcount == 0) {
	      check_for_messages();
	      local_loopcount = 0xfff;
	  }

	  strings_ptr = strings_curr_off;

	  /* This is a kludge, non-portable, but it eliminates a pr call  */
	  /* Note that the size is 8 on suns. Is there a better way?      */
	  /* bcopy(strings_ptr,(char *)&str_head,sizeof(strings_header)); */
	  bp1 = (char *) &str_head;
	  /* The copying of the file index is done only on a match */
	  bp1[4] = strings_ptr[4]; bp1[5] = strings_ptr[5];
	  /* bp1[6] = strings_ptr[6]; bp1[7] = strings_ptr[7];     */

	  tlen = (unsigned short) str_head.str_len;

	  /* To catch database corruption, this is a sanity check */
	  if((tlen < 0) || (tlen > MAX_STRING_LEN)) {
	      plog(L_DB_ERROR,NULL,NULL,"Database corrupt: string length out of bounds",0);
	      break;
	  }

	  /* Old code (replaced by inline code taken from strfind) */
	  /* if(strfind(strings_ptr,str_head.str_len))             */

	  if( tlen <= plen_1 ) continue;
	  pc = tc = plen_1;

	  strings_ptr += 8;

	  /* Moved the nocase test outside the inner loop for performace */
	  /* Clauses are identical except for the first if               */
	  if(nocase) do {
	      tchar = strings_ptr[tc];

	      /* improve efficiency of this test */
	      if(lowertable[tchar] == pattern[pc]) {--pc; --tc;}
	      else {
		  skip = skiptab[tchar] ;
		  tc += (skip < plen_1 - pc) ? plen : skip ;
		  pc = plen_1 ;
	      } 
	  } while( pc >= 0 && tc < tlen ) ;
	  else /* (!nocase) */ do {
	      tchar = strings_ptr[tc];

	      /* improve efficiency of this test */
	      if(tchar == pattern[pc]) {--pc; --tc;}
	      else {
		  skip = skiptab[tchar] ;
		  tc += (skip < plen_1 - pc) ? plen : skip ;
		  pc = plen_1 ;
	      } 
	  } while( pc >= 0 && tc < tlen ) ;

	  if(pc >= 0) continue;

	  /* We have a match */

	  /* Finish copying str_head - strings_curr_off */
	  /* is old strings_ptr.                        */
	  bp1[0] = strings_curr_off[0]; bp1[1] = strings_curr_off[1];
	  bp1[2] = strings_curr_off[2]; bp1[3] = strings_curr_off[3];

	  /* End heavily optimized and non portable code */

	  check_for_messages();

	  if(str_head.filet_index != -1){
	    retval = get_match_list((int) str_head.filet_index, max_hits + 
		                offset, &match_number, site_outptr, FALSE);

	    if((retval != A_OK) && (retval != HITS_EXCEEDED)) {
	      plog(L_DB_ERROR,NULL,NULL,"get_match_list failed (%d)",retval,0);
	      goto cleanup;
	    }

	    if( match_number >= max_hits + offset){
		hits_exceeded = TRUE;
		break;
	    }
	  }
      }
  }
      break;

  default:
      return(PRARCH_BAD_ARG);

  cleanup:
      for(i =  0;i <  match_number; i++) free((char *)site_outptr[i]);
      free((char *)site_outptr);
      return(PRARCH_DB_ERROR);
      
  }

    for(i =  0;i <  match_number; i++){
	if((i & 0x7f) == 0) check_for_messages();
	site_outrec = *site_outptr[i];
	if(i >= offset) {
	    cur_link = atoplink(site_outrec,archiedir);
	    if(cur_link) vl_insert(cur_link,vd,VLI_NOSORT);
	}
	free((char *)site_outptr[i]);
    }
    free((char *)site_outptr);

    if(hits_exceeded) {
      /* Insert a continuation entry */
    }
    
    if((search_type == S_EXACT) && (pQlen > (MATCH_CACHE_SIZE - 5)))
	return(PRARCH_SUCCESS);

    add_to_cache(vd->links,s_string, (hits_exceeded ? max_hits : -max_hits),
		 offset,search_type,or_search_type,archiedir);

    return(PRARCH_SUCCESS);
}


/* Check for cached results */
check_cache(arg,max_hits,offset,qtype,archiedir,linkpp)
    char	*arg;
    int		max_hits;
    int		offset;
    search_sel	qtype;
    int		archiedir;
    VLINK	*linkpp;
    {    
	struct match_cache 	*cachep = mcache;
	struct match_cache 	*pcachep = NULL;
	VLINK			tmp_link, cur_link;
	VLINK			rest = NULL;
	VLINK			next = NULL;
	int			count = max_hits;

	while(cachep) {
	    if(((qtype == cachep->search_type)||(qtype == cachep->req_type))&&
	       (cachep->offset == offset) &&
	       /* All results are in cache - or enough to satisfy request */
	       ((cachep->max_hits < 0) || (max_hits <= cachep->max_hits)) &&
	       (strcmp(cachep->arg,arg) == 0) &&
	       (cachep->archiedir == archiedir)) {
		/* We have a match.  Move to front of list */
		if(pcachep) {
		    pcachep->next = cachep->next;
		    cachep->next = mcache;
		    mcache = cachep;
		}

		/* We now have to clear the expanded bits or the links  */
		/* returned in previous queries will not be returned    */
		/* We also need to truncate the list of there are more  */
		/* matches than requested                               */
		cur_link = cachep->matches;
#ifdef NOTDEF   /* OLD code that is not maintained, it is unnecessary    */
		/* but it does show how we used to handle two-dimensions */
		while(cur_link) {
		    tmp_link = cur_link;
		    while(tmp_link) {
			tmp_link->expanded = FALSE;
			if(tmp_link == cur_link) tmp_link = tmp_link->replicas;
			else tmp_link = tmp_link->next;
		    }
		    cur_link = cur_link->next;
		}
#else /* One dimensional */
		/* IMPORTANT: This code assumes the list is one         */
		/* dimensional, which is the case because we called     */
		/* vl_insert with the VLI_NOSORT option                 */
		while(cur_link) {
		    cur_link->expanded = FALSE;
		    next = cur_link->next;
		    if(--count == 0) { /* truncate list */
			rest = cur_link->next;
			cur_link->next = NULL;
			if(rest) rest->previous = NULL;
		    }
		    else if((next == NULL) && (rest == NULL)) {
			next = cachep->more;
			cur_link->next = next;
			if(next) next->previous = cur_link;
			cachep->more = NULL;
		    }
		    else if (next == NULL) {
			cur_link->next = cachep->more;
			if(cur_link->next)
			    cur_link->next->previous = cur_link;
			cachep->more = rest;
		    }
		    cur_link = next;
		}
#endif
		*linkpp = cachep->matches;
		return(TRUE);
	    }
	    pcachep = cachep;
	    cachep = cachep->next;
	}
	*linkpp = NULL;
	return(FALSE);
    }

	
/* Cache the response for later use */
add_to_cache(vl,arg,max_hits,offset,search_type,req_type,archiedir)
    VLINK	vl;
    char	*arg;
    int		max_hits;
    int		offset;
    search_sel	search_type;
    search_sel	req_type;
    int		archiedir;
    {
      struct match_cache 	*newresults = NULL;
      struct match_cache 	*pcachep = NULL;

      if(cachecount < MATCH_CACHE_SIZE) { /* Create a new entry */
	newresults = (struct match_cache *) malloc(sizeof(struct match_cache));
	cachecount++;
	newresults->next = mcache;
	mcache = newresults;
	newresults->arg = stcopy(arg);
	newresults->max_hits = max_hits;
	newresults->offset = offset;
	newresults->search_type = search_type;
	newresults->req_type = req_type;
	newresults->archiedir = archiedir;
	newresults->matches = NULL;
	newresults->more = NULL;
    }
      else { /* Use last entry - Assumes list has at least two entries */
	  pcachep = mcache;
	  while(pcachep->next) pcachep = pcachep->next;
	  newresults = pcachep;

	  /* move to front of list */
	  newresults->next = mcache;
	  mcache = newresults;

	  /* Fix the last entry so we don't have a cycle */
	  while(pcachep->next != newresults) pcachep = pcachep->next;
	  pcachep->next = NULL;

	  /* Free the old results */
	  if(newresults->matches) {
	      newresults->matches->dontfree = FALSE;
	      vllfree(newresults->matches);
	      newresults->matches = NULL;
	  }
	  if(newresults->more) {
	      newresults->more->dontfree = FALSE;
	      vllfree(newresults->more);
	      newresults->more = NULL;
	  }

	  newresults->arg = stcopyr(arg,newresults->arg);
	  newresults->max_hits = max_hits;
	  newresults->offset = offset;
	  newresults->search_type = search_type;
	  newresults->req_type = req_type;
	  newresults->archiedir = archiedir;
      }

      /* Since we are caching the data.  If there are any links, */
      /* note that they should not be freed when sent back       */
      if(vl) vl->dontfree = TRUE;
    
      newresults->matches = vl;
  }
      

