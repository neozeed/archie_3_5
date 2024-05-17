/*
 * Copyright (c) 1991 by the University of Washington
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>
 */

#include <uw-copyright.h>
#include <time.h>

static		month_start[12] = {0, 31, 59, 90, 120, 151,
				   181, 212, 243, 273, 304, 334};

time_t
asntotime(timestring)
    char	*timestring;
{
    int		tmp;
    struct tm	ts;
    long		seconds;
    int		numleap;

    tmp = sscanf(timestring,"%4d%2d%2d%2d%2d%2d", &(ts.tm_year), 
                 &(ts.tm_mon), &(ts.tm_mday), &(ts.tm_hour), &(ts.tm_min),
                 &(ts.tm_sec)); 
    if(tmp != 6) return(0);

    ts.tm_mon = ts.tm_mon - 1;
    ts.tm_year = ts.tm_year - 1900;

    /* The following may not be portable */
    seconds = (ts.tm_year - 70) * 365 * 24 * 60 * 60;
    seconds = seconds + (month_start[ts.tm_mon] * 24 * 60 * 60);
    seconds = seconds + (ts.tm_mday * 24 * 60 * 60);
    seconds = seconds + (ts.tm_hour * 60 * 60);
    seconds = seconds + (ts.tm_min * 60);
    seconds = seconds + ts.tm_sec;

    /* Account for leap years (good until 2100) */
    numleap = ts.tm_year - 72;
    numleap = numleap / 4;
    if((ts.tm_mon > 1) && ((ts.tm_year % 4) == 0)) numleap++;
    seconds = seconds + (numleap * 24 * 60 * 60);

    return(seconds);
}
