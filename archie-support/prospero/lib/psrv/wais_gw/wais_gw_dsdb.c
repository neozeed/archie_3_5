/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-license.h>.
 */

#include <string.h>

#include <pmachine.h>
#include <psite.h>		/* for WAIS_SOURCE_DIR */

#include <ardp.h>
#include <pfs.h>
#include <pserver.h>
#include <psrv.h>
#include <perrno.h>
#include <plog.h>

/* Could be local files. */
#include "wprot.h"
#include "zutil.h"			/* for "any" */
#include "source.h"		/* for Source */
#include "inface.h"
#include "wais_gw_dsdb.h"
#include "ietftype.h"
#include "ietftype_parse.h"


/* Definitions of constants */
#define MAX_NUM_WAIS_LINKS       3000 /* maximum # of directory links to
                                           return before giving up. */

/*  ======= Some forward definitions ============ */
static void 
wsr_to_dirobj(WAISSearchResponse *wsr, P_OBJECT ob, char *host, char *port);
static VLINK DocHeader2vl(WAISDocumentHeader *wdh, char *host, char *port);
char *
read_wais_contents(char *host, char *port, char *type, 
			char *database, char *docid);

/* This function is passed an empty object which has been initialized with
   oballoc().  It returns a possibly populated directory and
   DIRSRV_NOT_FOUND or PSUCCESS.  Note that the directory will need
   to have links within it freed, even if an error code is returned.  */

/* This sets p_warn_string and pwarn if a unrecognized record
   or diagnostic record is received.  That is good. */

/* Currently this routine doesnt handle all possible combinations of arguments*/
/* It handles:
Do a search
	hsoname= "WAIS-GW/<host>(<port>)/QUERY/<database>/<query>"

Do a search - alternative
	hsoname= "WAIS-GW/<host>(<port>)/QUERY/<database>"
	listopts.thiscompp[0] = <query>
	
Retrieve a document
	hsoname= "WAIS-GW/<host>(<port>)/<type>/<database>/<docid>"
	listopts.requested_attrs="+CONTENTS+"

Get attribute information about database
	hsoname= "WAIS-GW/<host>(<port>)/QUERY/<database>"
	listopts.requested_attrs="+#ALL+"
	listopts.thiscompp[0] = NULL
*/

int 
wais_gw_dsdb(RREQ    req,     /* Request pointer (unused)           */
            char    *hsoname,   /* Name of the directory                 */
            long version,       /* Version #; currently ignored */
            long magic_no,      /* Magic #; currently ignored */
            int flags,          /* Currently only recognize DRO_VERIFY */
            struct dsrobject_list_options *listopts, /* options (use *remcompp
                                                        and *thiscompp) */
            P_OBJECT    ob)     /* Object to be filled in */
{
	int	tmp;
	AUTOSTAT_CHARPP(hostp);
	AUTOSTAT_CHARPP(portp);
	AUTOSTAT_CHARPP(queryp);
	AUTOSTAT_CHARPP(typep);
	AUTOSTAT_CHARPP(databasep);
	WAISSearchResponse *wsr;
	char *str = NULL;

    ob->version = 0;            /* unversioned */
    ob->magic_no = 0;
    ob->inc_native = VDIN_PSEUDO;
    ob->acl = NULL;
    tmp = qsscanf(hsoname, "WAIS-GW/%&[^(](%&[^)])/%&[^/]/%&[^/]/%&[^/]/%[^\n]",
              hostp, portp, typep, databasep, queryp);

    if (requested_contents(listopts)) {
        if (tmp == 4 && stequal(*typep,"HELP")) {
		char *db_src = NULL;
		WAISSOURCE thissource;
		db_src = qsprintf_stcopyr(db_src,"%s.src",*databasep);
		/* Dont free the source, its on a linked list for the duration*/
		if (!(thissource = findsource(db_src,WAIS_SOURCE_DIR)))
			return DIRSRV_WAIS;
		ob_atput(ob,"CONTENTS","DATA",
			thissource->description, (char *)0);
		stfree(db_src);
		return PSUCCESS;
	}
	if (tmp <5) 
		return DIRSRV_WAIS;
	ob->flags = P_OBJECT_FILE;
	if (!(str = read_wais_contents(*hostp,*portp,*typep,*databasep,*queryp))) 
		return DIRSRV_WAIS;
	/* (char *)1 prevents copying of potentially huge str*/
	ob_atput(ob, "CONTENTS", "DATA", (char *)1, str, (char *)0);
	return PSUCCESS;
    }
	
    ob->flags = P_OBJECT_DIRECTORY;
    if (tmp < 4) 
		return DIRSRV_NOT_FOUND;

    /* For now only handle QUERY */
    if (strcmp(*typep,"QUERY") != 0) 
		return DIRSRV_NOT_FOUND; 
	
    if (tmp <5) {	/* look for query in components */
        if (!listopts || !listopts->thiscompp || !*listopts->thiscompp ||
            strequal(*listopts->thiscompp, "")
            || strequal(*listopts->thiscompp, "*"))  {
            /* last test against * might be inappropriate  */
	    if (listopts->requested_attrs) {
		char *w_db_src = NULL;	
		WAISSOURCE thissource;	/* DONT free this */
		w_db_src = qsprintf_stcopyr(w_db_src,"%s.src",*databasep);
		ob_atput(ob,"OBJECT-INTERPRETATION", "SEARCH", NULL);
		if (thissource = findsource(w_db_src,WAIS_SOURCE_DIR)) {
		    if (thissource->subjects)
			ob_atput(ob,"WAIS-SUBJECTS", thissource->subjects, NULL);
		    if (thissource->cost)
			ob_atput(ob,"WAIS-COST-NUM", thissource->cost, NULL);
		    if (thissource->units)
			ob_atput(ob,"WAIS-COST-UNIT", thissource->units, NULL);
		    if (thissource->maintainer)
			ob_atput(ob,"WAIS-MAINTAINER", thissource->maintainer, NULL);
        	    ob_atput(ob, "QUERY-METHOD", "wais-query(search-words)", 
              		"${search-words}", "", (char *) 0);
        	    ob_atput(ob, "QUERY-ARGUMENT", "search-words", 
              		"Index word(s) to search for", "mandatory char*", 
			"%s", "", (char *) 0);
		    if (thissource->description)
            		ob_atput(ob, "QUERY-DOCUMENTATION", "wais-query()", 
			    thissource->description, (char *) 0);
        	    ob_atput(ob, "QUERY-DOCUMENTATION", "search-words", 
			"This string says \
what you're searching for or what information you're specifying.", 
             	"Type any string.  Sometimes SPACEs are treated as implied \
'and' operators.  Sometimes the words 'and', 'or', and 'not' are recognized \
as boolean operators.", (char *) 0);
		}
		stfree(w_db_src);
	    }
            return PSUCCESS;    /* Display no contents for the directory */
	}

	/* Set the selector. */
	*queryp = stcopyr(*listopts->thiscompp, *queryp);
	/* We just used up thiscompp, so we'd better reset it. */
	if (listopts->remcompp && *listopts->remcompp) {
		*listopts->thiscompp = (*listopts->remcompp)->token;
            	*listopts->remcompp = (*listopts->remcompp)->next;
        } else {
            *listopts->thiscompp = NULL;
        }
    }
	
	if (flags & DRO_VERIFY) return PSUCCESS;
	if ((wsr = waisQuery(*hostp,*portp,*databasep,*queryp)) == NULL)
		return DIRSRV_WAIS; /* Need to decode wais errors sensibly */
	wsr_to_dirobj(wsr,ob,*hostp,*portp);
	freeWAISSearchResponse(wsr); wsr=NULL;
    	return PSUCCESS;
}


static void 
wsr_to_dirobj(WAISSearchResponse *wsr, P_OBJECT ob, char *host, char *port)
{
/* Dont need to fuss around with magic numbers, these cant be replicas*/

    PATTRIB at = atalloc();
    VLINK vl = NULL;
    int i;
    long current_magic_no = 0L; 	/* set magic starting hash */
    VLINK tempvl = NULL;

    ob_atput(ob,"WAIS_SEED_WORDS",wsr->SeedWordsUsed, (char *)0);
    if  (  wsr->ShortHeaders
	|| wsr->LongHeaders
	|| wsr->Text
	|| wsr->Headlines
	|| wsr->Codes
	|| wsr->Diagnostics
	) {
	pwarn = PWARNING;
	p_warn_string = qsprintf_stcopyr(p_warn_string, 
		"Unrecognized search response");
    }
    /*!! Need to handle diagnostics - ignore others */
    if (wsr->DocHeaders) {
	for (i=0; wsr->DocHeaders[i]; i++) {
	  if (vl = DocHeader2vl(wsr->DocHeaders[i],host,port)) {
	    /* Add new magic no to all the links returned */
	    current_magic_no = generate_magic(vl);
	    while (magic_no_in_list(++current_magic_no, ob->links));
	    for (tempvl = vl; tempvl != NULL; tempvl= tempvl->next)
#ifdef NEVERDEFINED
		tempvl->f_magic_no = current_magic_no;
#else
	       	tempvl->f_magic_no = 0; /*clients dont understand result !!*/
#endif
	    APPEND_LISTS(ob->links, vl);
	}
    }
    }
}

char *
urlascii(any *docid) {
    char	*str = stalloc(3 * docid->size);
    int		i;
    int		j;

    for (i=0, j=0; i< docid->size; i++) 
    {
	int c = docid->bytes[i];
	if ((c < ' ') || (c > (char)126) || strchr("/\\{}|[]^~<>#%'",c)) {
		str[j++] = '%' ; 
		sprintf(str+(j++),"%02x",c);
		j++;
	} else
		str[j++] = c;
    }
    str[j] = '\0';	/* Null terminated */
    return str;
};

static VLINK
DocHeader2vl(WAISDocumentHeader *wdh, char *host, char *port)
{
    int i;	/* Index into types */
    VLINK vl;
    VLINK head = NULL;	/* Head of links being constructed */
    char *str = NULL;	/* thisstring can be stalloc-ed or stfree-ed*/
    char *cp = NULL;	/* this one cant */
    IETFTYPE it;
    PATTRIB at;
    for (i=0; wdh->Types[i]; i++) {
	vl = vlalloc();
	vl->host = stcopy(hostwport);
	str = urlascii(wdh->DocumentID); 	/* stalloc's string */
	if ((cp=strrchr(wdh->Source,'/')) == NULL) {
		cp = wdh->Source;
	} else {
		cp++;
	}
	vl->hsoname = qsprintf_stcopyr(vl->hsoname,"WAIS-GW/%s(%s)/%s/%s/%s",
		host, port, wdh->Types[i], cp, str);
/*!! Check that Source always has a reasonable format */
	if (it = wais_2_ietftype(wdh->Types[i])) {
	     if (at = atcopy(it->prosperotype)) {
		APPEND_ITEM(at,vl->lattrib);
	     }
	}
	vl->name = stcopy(wdh->Headline);
	if (wdh->VersionNumber)
		vl->version = wdh->VersionNumber;
	if (wdh->Score) {
	    vl_atput(vl, "WAIS-SCORE", 
		(str = qsprintf_stcopyr(str,"%d",wdh->Score)), (char *)0);
	    vl_atput(vl, "COLLATION-ORDER",
		(str = qsprintf_stcopyr(str,"-%d",wdh->Score)), (char *)0);
	}
	if (wdh->BestMatch)
	    vl_atput(vl, "WAIS-BESTMATCH", 
		(str = qsprintf_stcopyr(str,"%d",wdh->BestMatch)), (char *)0);
	    vl_atput(vl, "SIZE", 
		(str = qsprintf_stcopyr(str,"%d",wdh->BestMatch)), (char *)0);
	if (wdh->Lines) 
	    vl_atput(vl, "WAIS-LINES", 
		(str = qsprintf_stcopyr(str,"%d",wdh->Lines)), (char *)0);
/*!! Should convert this date to asn (timetoasn & lib/psrv/dsrfinfo*/
/*   and stuff in LAST-MODIFIED */
	if (wdh->Date)
	    vl_atput(vl, "WAIS-DATE", wdh->Date, (char *)0);
	if (wdh->OriginCity)
	    vl_atput(vl, "WAIS-ORIGINCITY", wdh->OriginCity, (char *)0);
	/* Route all accesses through the host for now */
	vl_atput(vl,"ACCESS-METHOD","WAIS","","","","", (char *)0);
	/*vl_atput(vl,"ACCESS-METHOD","PROSPERO-CONTENTS","","","","", (char *)0);*/
	if (strncmp("Search produced no result. Here's the Catalog",
			vl->name, 45) == 0) {
		vlfree(vl);
	} else {
	APPEND_ITEM(vl,head);
	}
    stfree(str);
    } /*for*/
    return(head);
}

char *
read_wais_contents(char *host, char *port, char *type, 
			char *database, char *docid)
{
    any			*dt;
    char		*str = NULL;
    int			len;
    any			*DocumentId;

    DocumentId = un_urlascii(docid);  /* Allocates DocumentID (throws length)*/
    len = 0 ;
    if (dt = waisRetrieve(host,port,database,DocumentId,type,len)) {
    str = dt->bytes;
    dt->bytes = NULL;
    freeAny(dt);
    }
    stfree(DocumentId->bytes);
    free(DocumentId);
    return (str);
}

