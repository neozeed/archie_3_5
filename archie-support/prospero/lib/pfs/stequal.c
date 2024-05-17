/* Copyright (c) 1992 -- 1993by the University of Southern California.
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 */

#include <usc-copyr.h>
#include <pfs.h>

/* Equality tester for strings.  This is used because there are a number of
   places in the Prospero code where string pointersare left set to NULL.
   Rather than paranoically guard against all such places, we can just use this
   function to test string equality, where appropriate.
*/

int
stequal(const char *s1, const char *s2)
{
    if (s1 == s2)          /* test for case when both NULL*/
        return TRUE;
    if (!s1 || !s2)        /* Test for case where one is NULL; other is not */
        return FALSE;
    return strequal(s1, s2);
}
