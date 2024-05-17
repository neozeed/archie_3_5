#include <stdio.h>
#include <string.h> /* memset() */
#include <time.h>
#include "defs.h"
#include "error.h"
#include "misc.h"
#include "ppc_time.h"
#include "protos.h"
#include "str.h"
#include "all.h"


static time_t cvt_to_inttime proto_((const char *datestring, int islocal));


/*
 * cvt_to_inttime: convert a string of the form YYYYMMDDHHMMSS to internal
 * format (seconds since 1 Jan 1970).
 */

static time_t cvt_to_inttime(datestring, islocal)
  const char *datestring; /* String to be converted */
  int islocal;            /* non-zero if the time being given is in the local time, not UTC */
{
#if defined(AIX) || defined(SOLARIS)
  extern long timezone;
#endif

  long the_tz = 0;
  struct tm *local_tm;
  struct tm input_time;         /* structure to pass to conversion routines */

  if ( ! islocal)
  {
#if defined(AIX) || defined(SOLARIS)
    tzset();
    the_tz = timezone * timezone_sign();
#else
    time_t mytime = time((time_t *)0);

    local_tm = localtime(&mytime);
    the_tz = local_tm->tm_gmtoff;
#endif
  }

  memset((char *)&input_time, 0, sizeof input_time);
  if (sscanf((char *)datestring,"%4u%2u%2u%2u%2u%2u",
             &input_time.tm_year,
             &input_time.tm_mon,
             &input_time.tm_mday,
             &input_time.tm_hour,
             &input_time.tm_min,
             &input_time.tm_sec) != 6)
  {
    efprintf(stderr, "%s: cvt_to_inttime: time string, `%s', not in `YYYYMMDDHHMMSS' format.\n",
             logpfx(), datestring);
    return (time_t)0;
  }

  /* timegm() expects the year field to be year - 1900 */

  if (input_time.tm_year != 0)
  {
    input_time.tm_year -= 1900;
  }
#ifdef AIX
  else
  {
    input_time.tm_year = 70;      /* 1970, the epoch */
  }
#endif

  /* Months are counted from 0 */

  if (input_time.tm_mon != 0) /* bug? isn't this always true?! */
  {
    input_time.tm_mon--;
  }

  if (input_time.tm_mday == 0) /* bug: same here? */
  {
    input_time.tm_mday = 1;
  }

#if !defined(AIX) && !defined(SOLARIS)
  return timelocal(&input_time) + (islocal ? 0 : the_tz);
#else
  return mktime(&input_time) + (islocal ? 0 : the_tz);
#endif
}


/*
 *  Like strftime(), but pass it a Prospero time string, instead.
 */

int pstrftime(buf, bufsize, fmt, ptime)
  char *buf;
  int bufsize;
  const char *fmt;
  const char *ptime;
{
  time_t tt;

  if ((tt = cvt_to_inttime(ptime, 0)))
  {
    struct tm *tlocal = localtime(&tt);
    return strftime(buf, bufsize, (char *)fmt, tlocal);
  }
  else
  {
    strncopy(buf, "<bad time>", bufsize - 1);
    return 0;
  }
}
