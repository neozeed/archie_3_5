/*
 * Copyright (c) 1989, 1990 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>
/*----------------------------------------------------------------------*/
/*									*/
/*				helpers.c				*/
/*									*/
/*	    Routines to help Prospero interface with the EI.		*/
/*									*/
/*----------------------------------------------------------------------*/

#include        "ei/arbBA/ei.h"
#include        "ei/arbBA/symtab.h"
#include        "ei/arbBA/node.h"
#include        "ei/arbBA/arb.h"
#include        "ei/arbBA/map.h"
#include	<pfs.h>
#include	<stdio.h>
#include	<string.h>

/*typedef char *(*STR_function)();*/

STR_function filter;


/*
 * load_filter - a crude but adequate procedure for loading in a filter.
 *  When called several times in succession, cascades the filters
 *  in nested `focuses'.  This assures that the currently loaded file
 *  is the one first looked at, and then it searches upward.  A better
 *  model might build a very bushy tree instead, which would prevent a
 *  bad reference being accidentally resolved by a filter loaded earlier.
 */
load_filter(filter_file_name)
char *filter_file_name;
    {
        struct node *app_node;
	struct node *ext_node;

	int	    fnl;  /* Filter name length */

	fnl = strlen(filter_file_name);

	if(strcmp(filter_file_name+fnl-2,".o")==0) {
	    *(filter_file_name+fnl-2) = '\0';
	}
	    
	app_node = EIRegister(CURRENT,filter_file_name);
	ext_node = ext(app_node);
	change_focus(ext_node);
    }


STR_function select_filter(filter_name)
char *filter_name;
   {
   return (STR_function) C_select(CURRENT,filter_name);
   }


initialize_loader(fullpath)
char *fullpath;
    {
    ei_initialize(1,&fullpath);
    }

struct sym_ent {
    char *symbol;
    long addr;
};

/* The following is the list of variables and procedures that */
/* are to be accessible from within filters.                  */
/*                                                            */
/* The values of these variables should be constant, though   */
/* an errant filter might change them.                        */

extern int 	atlfree();
extern char 	*index();
extern PATTRIB 	pget_at();
extern int 	printf();
extern VLINK 	rd_vlink();
extern char 	*stcopy();
extern char 	*stcopyr();
extern char 	*strncpy();
extern VLINK 	vlalloc();
extern VLINK 	vlcopy();
extern int 	vllfree();
extern VLINK 	vl_delete();
extern int 	vl_insert();
extern int 	wcmatch();
extern char 	*FlT_htype; 
extern char 	*FlT_ostype; 

static struct sym_ent 
filter_externals[] = {{"_atlfree",(long)atlfree},
	      	      {"_index",(long)index},
		      {"_pget_at",(long)pget_at},
		      {"_printf",(long)printf},
		      {"_rd_vlink",(long)rd_vlink},
		      {"_sprintf",(long)sprintf},
		      {"_stcopy",(long)stcopy},	
		      {"_stcopyr",(long)stcopyr},
		      {"_vlalloc",(long)vlalloc},
		      {"_vlcopy",(long)vlcopy},
		      {"_vllfree",(long)vllfree},
		      {"_vl_delete",(long)vl_delete},
		      {"_vl_insert",(long)vl_insert},
		      {"_wcmatch",(long)wcmatch},
		      {"_FlT_hosttype",(long)&FlT_htype},
		      {"_FlT_ostype",(long)&FlT_ostype},
		      {"",0}};

/*
 * symbol_find - take a name passed from the C dynamic linker and look it
 * up in this table for prospero filters.  Returns its address, or -1 on 
 * failure.
 */
long symbol_find(name)
char *name;
    {
	struct sym_ent *sym;
	for (sym = filter_externals ; sym->addr != 0; sym++) {
	    if (strcmp(name,sym->symbol) == 0)
		return(sym->addr);
	}
	return -1;	/* failure */
    }

