/* Build an attribute

   Author - Mitra

   (Steve - feel free to incorporate as you wish)
*/

#include <pfs.h>


/* Build a PATTRIB, with its list of tokens */
/* Note convention that calling routing passes last arg as (char *)0  */
/* Also note , that passing (char *)1 will force the next argument to 
   be pointed to, rather than copied (which is what tkappend does) */
/* Return it - this fills in defaults, caller can override */
PATTRIB 
vatbuild(char *name, va_list ap)
{
    char *s;
    PATTRIB at = atalloc();

    at->aname = stcopy(name);
    /* If any of these arent reasonable defaults move up to callers */
    at->avtype = ATR_SEQUENCE;
    if (strequal(name, "ACCESS-METHOD"))
        at->nature = ATR_NATURE_FIELD;
    else if strequal(name,"CONTENTS")
	at->nature = ATR_NATURE_INTRINSIC;
    else
        at->nature = ATR_NATURE_APPLICATION;
    at->precedence = ATR_PREC_OBJECT;
    while(s = va_arg(ap, char *)) {
	if (s == (char *) 1) {
		at->value.sequence = 
			tkappend(NULL,at->value.sequence);
		s = va_arg(ap, char *);
		at->value.sequence->previous->token = s;
	} else 
        	at->value.sequence = 
            		tkappend(s, at->value.sequence);
    }

    return(at);
}

