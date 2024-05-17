/* Other routines - written by Mitra - that deal with P_OBJECT's
 *
 * Steve - feel free to move anywhere you want
 */
#include <pfs.h>

/* Syntactically same as atput in goph_gw_dsdb.c */
/* Note caller must pass last arg as  (char *)0  */
/* Note an arg of *char *)1 will force next arg to be pointed at rather
   than copied */
void ob_atput(P_OBJECT po, char *name, ...)
{
    va_list ap;
    PATTRIB at;

    va_start(ap, name);
    at = vatbuild(name, ap);
    APPEND_ITEM(at, po->attributes);
    va_end(ap);
}

