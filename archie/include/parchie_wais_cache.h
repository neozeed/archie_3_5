#ifndef _PARCHIE_WAIS_CACHE_H_
#define _PARCHIE_WAIS_CACHE_H_

#include "prwais_do_search.h"

struct wais_cache{
    char                *arg;	     /* WAIS search string                  */
    char		*db;	     /* Databases of search		    */
    int			expand;	     /* status of expand flag		    */
    int			lmodtime;    /* Last modified time of database	    */
    int			maxdoc;      /* Maximum matchess <0 = found all     */
    int			maxheader;   /* Maximum matchess <0 = found all     */
    VLINK		matches;     /* Matches                             */
    VLINK		more;	     /* Additional matches                  */
    struct wais_cache 	*next;       /* Next entry in cache                 */
} ;

typedef struct wais_cache wais_cache_t;


struct doc_cache{
   int	    size;		/* size of docid */
   char	    *docid;		/* docid         */
   char     *bytes;		/* bytes of doc  */
   char	    *seqstr;		/* headline	 */
   int	    lut;		/* Last used time*/
   struct doc_cache *next;      /* next structure*/
};

#define WAIS_DOC_CACHESIZE	  200	     /* store the last 20 documents */

typedef struct doc_cache doc_cache_t;


extern status_t wais_doc_cache_add PROTO((char *, int *, char *, char *));
extern char     *wais_doc_check_cache PROTO((char *, int *, char *));

extern int     wais_check_cache PROTO((wais_search_req_t *, VLINK *, wais_cache_t *));
extern int     wais_add_to_cache PROTO((VLINK,wais_search_req_t *, wais_cache_t **));


#endif
