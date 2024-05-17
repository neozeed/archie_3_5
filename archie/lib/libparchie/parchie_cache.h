#ifndef _PARCHIE_CACHE_H_
#define _PARCHIE_CACHE_H_

struct match_cache{
    char                *arg;	     /* Matched regular expression          */
    char		*domains;    /* Domains asked for		    */
    char		*comp_rest;  /* component restrictions		    */
    int			maxhits;    /* Maximum matchess <0 = found all     */
    int			maxmatch;    /* Maximum matchess <0 = found all     */
    int			maxhitpm;    /* Maximum matchess <0 = found all     */
    int			offset;      /* Offset                              */
    search_sel_t	search_type; /* Search method (the one used)        */
    search_sel_t        req_type;    /* Requested method                    */
    VLINK		matches;     /* Matches                             */
    VLINK		more;	     /* Additional matches                  */
    int			archiedir;   /* Flag: directory links are to archie */
    struct match_cache 	*next;       /* Next entry in cache                 */
} ;

typedef struct match_cache match_cache_t;

extern int     check_cache PROTO((search_req_t *,VLINK	*,char *,char *,match_cache_t *));
extern int     add_to_cache PROTO((VLINK, search_req_t *,char *,char *,match_cache_t **,int *));


#endif
