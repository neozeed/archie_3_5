/*
 * Copyright (c) 1989 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <uw-copyright.h>.
 */

#include <uw-copyright.h>

/*
 * month_sname - Return a month name from it's number
 *
 *               MONTH_SNAME takes a number in the range 0
 *               to 12 and returns a pointer to a string
 *               representing the three letter abbreviation
 *	         for that month.  If the argument is out of 
 *		 range, MONTH_SNAME returns a pointer to "Unk".
 *
 *       ARGS:   n - Number of the month
 *    RETURNS:   Abbreviation for selected month
 */
char *month_sname(n)
    int n;		/* Month number */
{
    static char *name[] = {"Unk",
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    return((n < 1 || n > 12) ? name[0] : name[n]);
}
