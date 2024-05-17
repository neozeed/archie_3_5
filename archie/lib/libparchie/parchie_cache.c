#include <stdio.h>
#include <malloc.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pfs.h"
#include "typedef.h"
#include "ar_search.h"
#include "parchie_cache.h"
#ifdef WAIS_SUPPORT
#include "parchie_wais_cache.h"
#endif
#include "error.h"
#include "protos.h"


/* Check for cached results */
int check_cache(search_req, linkpp, domains, comp_rest, mcache)
  search_req_t *search_req;
  VLINK *linkpp;
  char *domains;
  char *comp_rest;
  match_cache_t *mcache;
{    
	VLINK cur_link;
	int count;
	int maxhitpm = search_req->maxhitpm;
	int maxhits = search_req->maxhits;
	int maxmatch = search_req->maxmatch;
	int offset;
  /* int archiedir = GET_ARCHIE_DIR(search_req->attrib_list); */
	match_cache_t *cachep = mcache;
	match_cache_t *pcachep = NULL;
	search_sel_t qtype;
  char *arg;

	count = maxhits > maxmatch * maxhitpm ? maxmatch * maxhitpm : maxhits;
	arg = search_req->search_str;
	qtype = search_req->curr_type;
	offset = search_req->orig_offset;

	while (cachep)
  {
    if (((qtype == cachep->search_type) || (qtype == cachep->req_type)) &&
        (cachep->offset == offset) &&
        /* All results are in cache - or enough to satisfy request */
        ((cachep->maxhits < 0) || (maxhits <= cachep->maxhits)) &&
        ((cachep->maxmatch < 0) || (maxmatch <= cachep->maxmatch)) &&
        ((cachep->maxhitpm < 0) || (maxhitpm <= cachep->maxhitpm)) &&	       
        (strcmp(cachep->arg,arg) == 0) &&
        /* (cachep->archiedir == archiedir) && */
        (strcmp(cachep->domains, domains) == 0) &&
        (strcmp(cachep->comp_rest, comp_rest) == 0))
    {
      /* We have a match.  Move to front of list */
      if (pcachep)
      {
		    pcachep->next = cachep->next;
		    cachep->next = mcache;
		    mcache = cachep;
      }

      /* We now have to clear the expanded bits or the links  */
      /* returned in previous queries will not be returned    */
      /* We also need to truncate the list if there are more  */
      /* matches than requested                               */

      cur_link = cachep->matches;

      /* IMPORTANT: This code assumes the list is one         */
      /* dimensional, which is the case because we called     */
      /* vl_insert with the VLI_NOSORT option                 */

      while (cur_link)
      {
		    cur_link->expanded = FALSE;
		    if ((--count == 0) && cur_link->next)
        {
          /* truncate list */
          if (cachep->more)
          {
            cur_link->next->previous = cachep->more->previous;
            cachep->more->previous = cachep->matches->previous;
            cachep->matches->previous->next = cachep->more;
          }
          else
          {
            cachep->more = cur_link->next;
            cachep->more->previous = cachep->matches->previous;
          }
          cur_link->next = NULL;
          cachep->matches->previous = cur_link;
		    }
		    else if ((cur_link->next == NULL) && (count != 0) &&
                 cachep->more)
        {
          /* Merge lists */
          cachep->matches->previous = cachep->more->previous;
          cur_link->next = cachep->more;
          cachep->more->previous = cur_link;
          cachep->more = NULL;
		    }
		    cur_link = cur_link->next;
      }
      *linkpp = cachep->matches;
      return(TRUE);
    }
    pcachep = cachep;
    cachep = cachep->next;
	}
	*linkpp = NULL;
	return FALSE;
}

	
/* Cache the response for later use */
int add_to_cache(vl, search_req, domains, comp_rest, mcache, cachecount)
    VLINK	vl;
    search_req_t *search_req;
    char        *domains;
    char        *comp_rest;
    match_cache_t **mcache;
    int	*cachecount;
    {
      match_cache_t 	*newresults = NULL;
      match_cache_t 	*pcachep = NULL;
      match_cache_t	*mycache = *mcache;
      int		matches;	
      VLINK		vptr;

      if(*cachecount < MATCH_CACHE_SIZE) { /* Create a new entry */
	for(vptr = vl, matches = 0; vptr; vptr = vptr -> next, matches++);
	newresults = (struct match_cache *) malloc(sizeof(struct match_cache));
	(*cachecount)++;
	newresults->next = mycache;
	*mcache = newresults;
	newresults->arg = stcopy(search_req -> search_str);
	newresults->domains= stcopy(domains);
	newresults->comp_rest= stcopy(comp_rest);
	newresults->maxhits = matches < search_req -> maxhits ? -1 : matches;
	newresults->maxhitpm = search_req -> maxhitpm;
	newresults->maxmatch = search_req -> maxmatch;
	newresults->offset = search_req -> orig_offset;
	newresults->search_type = search_req -> orig_type;
	newresults->req_type = search_req -> curr_type;
	newresults->archiedir = GET_ARCHIE_DIR(search_req -> attrib_list);
	newresults->matches = NULL;
	newresults->more = NULL;
    }
      else { /* Use last entry - Assumes list has at least two entries */
	  pcachep = mycache;
	  while(pcachep->next) pcachep = pcachep->next;
	  newresults = pcachep;

	  for(vptr = vl, matches = 0; vptr; vptr = vptr -> next, matches++);

	  /* move to front of list */
	  newresults->next = mycache;
	  *mcache = newresults;

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

	  newresults->arg = stcopyr(search_req -> search_str,newresults->arg);
	  newresults->maxhits = matches < search_req -> maxhits ? -1 : matches;
	  newresults->maxhitpm = search_req -> maxhitpm;
	  newresults->maxmatch = search_req -> maxmatch;
	  newresults->offset = search_req -> orig_offset;
	  newresults->search_type = search_req -> orig_type;
	  newresults->req_type = search_req -> curr_type;
	  newresults->archiedir = GET_ARCHIE_DIR(search_req -> attrib_list);
	  newresults->domains= stcopyr(domains, newresults->domains);
	  newresults->comp_rest= stcopyr(comp_rest, newresults->comp_rest);
      }

      /* Since we are caching the data.  If there are any links, */
      /* note that they should not be freed when sent back       */
      if(vl) vl->dontfree = TRUE;
    
      newresults->matches = vl;
      return(0);
  }




#ifdef WAIS_SUPPORT

/* Check for cached results */
int wais_check_cache(search_req, linkpp, mcache)
  wais_search_req_t *search_req;
  VLINK *linkpp;
  wais_cache_t *mcache;
{    
   struct stat statbuf;
   pathname_t path_name;
   VLINK cur_link;
   int count;
   int maxdoc = search_req->Max_Docs;
   int maxheader = search_req->Max_Headers;
   wais_cache_t *cachep = mcache;
   wais_cache_t *pcachep = NULL;
   char *arg;

   count = maxdoc;
   arg = search_req->keyword_list;

   statbuf.st_mtime = 0;

   sprintf(path_name, "%s.inv", search_req -> database);

   if(stat(path_name, &statbuf) == -1){

      error(A_ERR, "wais_add_to_cache", "Can't stat %s database", search_req -> database);
   }

   while (cachep){
    if ((strcmp(arg, cachep -> arg) == 0) &&
        /* All results are in cache - or enough to satisfy request */
	(strcmp(search_req -> database, cachep-> db) == 0) &&
	(cachep -> lmodtime >= statbuf.st_mtime) &&
	(cachep -> expand >= search_req -> expand) &&
	((cachep->maxdoc < 0) || (maxdoc <= cachep->maxdoc)) &&
        ((cachep->maxheader < 0) || (maxheader <= cachep->maxheader)))
    {
      /* We have a match.  Move to front of list */
      if (pcachep)
      {
		    pcachep->next = cachep->next;
		    cachep->next = mcache;
		    mcache = cachep;
      }

      /* We now have to clear the expanded bits or the links  */
      /* returned in previous queries will not be returned    */
      /* We also need to truncate the list if there are more  */
      /* matches than requested                               */

      cur_link = cachep->matches;

      /* IMPORTANT: This code assumes the list is one         */
      /* dimensional, which is the case because we called     */
      /* vl_insert with the VLI_NOSORT option                 */

      while (cur_link)
      {
	    cur_link->expanded = FALSE;
	    if ((--count == 0) && cur_link->next)
        {
          /* truncate list */
          if (cachep->more)
          {
            cur_link->next->previous = cachep->more->previous;
            cachep->more->previous = cachep->matches->previous;
            cachep->matches->previous->next = cachep->more;
          }
          else
          {
            cachep->more = cur_link->next;
            cachep->more->previous = cachep->matches->previous;
          }
          cur_link->next = NULL;
          cachep->matches->previous = cur_link;
		    }
		    else if ((cur_link->next == NULL) && (count != 0) &&
                 cachep->more)
        {
          /* Merge lists */
          cachep->matches->previous = cachep->more->previous;
          cur_link->next = cachep->more;
          cachep->more->previous = cur_link;
          cachep->more = NULL;
		    }
		    cur_link = cur_link->next;
      }
      *linkpp = cachep->matches;
      return(TRUE);
    }
    pcachep = cachep;
    cachep = cachep->next;
	}
	*linkpp = NULL;
	return FALSE;
}

	
/* Cache the response for later use */
int wais_add_to_cache(vl, search_req, mcache)
   VLINK	vl;
   wais_search_req_t *search_req;
   wais_cache_t **mcache;
{
   static int	cachecount;
   pathname_t path_name;
   struct stat statbuf;
   wais_cache_t 	*newresults = NULL;
   wais_cache_t 	*pcachep = NULL;
   wais_cache_t	*mycache = *mcache;
   int		matches;	
   VLINK		vptr;

   /* BUG -- depends on the names of WAIS index files */

   sprintf(path_name, "%s.inv", search_req -> database);

   if(cachecount < MATCH_CACHE_SIZE) { /* Create a new entry */
      for(vptr = vl, matches = 0; vptr; vptr = vptr -> next, matches++);
      newresults = (struct wais_cache *) malloc(sizeof(struct match_cache));
      cachecount++;
      newresults->next = mycache;
      *mcache = newresults;
      newresults-> expand = search_req -> expand;
      newresults->arg = stcopy(search_req -> keyword_list);
      newresults->db = stcopy(search_req -> database);
      newresults->maxdoc = matches < search_req -> Max_Docs ? -1 : matches;
      newresults->maxheader = search_req -> Max_Headers;

      if(stat(path_name, &statbuf) == -1){

	 error(A_ERR, "wais_add_to_cache", "Can't stat %s database", search_req -> database);
      }
      else
	 newresults -> lmodtime = statbuf.st_mtime;

      newresults->matches = NULL;
      newresults->more = NULL;
   }
   else { /* Use last entry - Assumes list has at least two entries */
      pcachep = mycache;
      while(pcachep->next) pcachep = pcachep->next;
      newresults = pcachep;

      for(vptr = vl, matches = 0; vptr; vptr = vptr -> next, matches++);

      /* move to front of list */
      newresults->next = mycache;
      *mcache = newresults;

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

      newresults->arg = stcopyr(search_req -> keyword_list,newresults->arg);
      newresults->db = stcopyr(search_req -> database, newresults->db);
      newresults->maxdoc = matches < search_req -> Max_Docs ? -1 : matches;
      newresults->maxheader = search_req -> Max_Headers;
      newresults-> expand = search_req -> expand;

      if(stat(path_name, &statbuf) == -1){
	 error(A_ERR, "wais_add_to_cache", "Can't stat %s database", search_req -> database);
      }
      else
	 newresults -> lmodtime = statbuf.st_mtime;
   }

   /* Since we are caching the data.  If there are any links, */
   /* note that they should not be freed when sent back       */
   if(vl) vl->dontfree = TRUE;

   newresults->matches = vl;
   return(0);
}


status_t wais_doc_cache_add(docid, size, bytes, seqstr)
   char *docid;
   int  *size;
   char *bytes;
   char *seqstr;
{
   extern doc_cache_t *doc_cache;
   static int cachecount;
   doc_cache_t *a;
   doc_cache_t *b;

   if(cachecount < WAIS_DOC_CACHESIZE){

      if(doc_cache == (doc_cache_t *) NULL){
	 if((a = doc_cache = (doc_cache_t *) malloc(sizeof(doc_cache_t))) == (doc_cache_t *) NULL)
	    return(ERROR);
      }
      else{

	 for(a = doc_cache; a != (doc_cache_t *) NULL;){
	    b = a;
	    a = a -> next;
	 }

	 if((b -> next = (doc_cache_t *) malloc(sizeof(doc_cache_t))) == (doc_cache_t *) NULL)
	    return(ERROR);

	 a = b -> next;
      }

      a -> size = *size;
      if((a -> docid = (char *) malloc(*size)) == (char *) NULL)
	 return(ERROR);

      memcpy(a -> docid, docid, *size);

      a -> lut = time((time_t) NULL);

      cachecount++;

      a -> bytes = strdup(bytes);

      a -> seqstr = strdup(seqstr);

      a -> next = (doc_cache_t *) NULL;

      return(A_OK);
   }
   else{
      int lru = INT_MAX;
      doc_cache_t *p;

      for(a = doc_cache, p = a, lru = 0; a != (doc_cache_t *) NULL;){

	 if(a -> lut < lru){
	    p = a;
	    lru = a -> lut;
	 }

	 a = a -> next;
      }

      free(p -> bytes);

      free(p -> seqstr);

      free(p -> docid);

      p -> size = *size;
      p -> lut = time((time_t *) NULL);

      if((p -> docid = (char *) malloc(*size)) == (char *) NULL)
	 return(ERROR);

      memcpy(p -> docid, docid, *size);

      p -> bytes = strdup(bytes);

      p -> seqstr = strdup(seqstr);

      return(A_OK);
   }
}


char *wais_doc_check_cache(docid, size, seqstr)
   char *docid;
   int *size;
   char *seqstr;
{
   extern doc_cache_t *doc_cache;

   doc_cache_t *a;
   char *q;

   for( a = doc_cache; a != (doc_cache_t *) NULL;){

      if((*size == a -> size) && (memcmp(a -> docid, docid, a -> size) == 0)){

	 /* have a match */

	 q = strdup(a -> bytes);

	 strcpy(seqstr, a -> seqstr);

	 a -> lut = time((time_t *) NULL);

	 return(q);
      }

      a = a -> next;
   }

   return((char *) NULL);
}

#endif /* WAIS SUPPORT */
