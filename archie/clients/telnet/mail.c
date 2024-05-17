#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include "ansi_compat.h"
#include "client_defs.h"
#include "error.h"
#include "extern.h"
#include "lang.h"
#include "macros.h"
#include "master.h" /* get_archie_hostname() */
#include "mail.h"
#include "mail_lang.h"
#include "misc.h"
#include "misc_ansi_defs.h"
#include "vars.h"

#include "protos.h"

/*
 *
 *
 *                                   Internal routines.
 *
 *
 */


static const char *std_mail_time PROTO((void));
static const char *tzstr PROTO((long offset));
  

static const char *tzstr(offset)
  long offset; /* in seconds */
{
  if (offset == 0)
  {
    return "GMT";
  }
  else
  {
    static char timestr[64];
    int tz_hr_diff = abs(offset) / (60 * 60);
    int tz_min_diff = (abs(offset) / 60) % 60;

    sprintf(timestr, "%c%02d%02d",
            offset < 0 ? '-' : '+',
            tz_hr_diff,
            tz_min_diff);
    return timestr;
  }
}


static const char *std_mail_time()
{
  char timestr[64];
  static char finalstr[64];
  struct tm *tptr;
  time_t t;
#if defined(AIX) || defined(SOLARIS)
  extern long timezone;
#endif  

  t = time((time_t *)0);
  tptr = localtime(&t);
  sprintf(timestr, "%s, %d %s %d %02d:%02d",
          days[tptr->tm_wday],
          tptr->tm_mday,
          months[tptr->tm_mon],
          tptr->tm_year,
          tptr->tm_hour,
          tptr->tm_min
          );
  sprintf(finalstr, "%s %s", timestr,

#if !defined(AIX) && !defined(SOLARIS)
          tzstr(tptr->tm_gmtoff)
#else
          tzstr(timezone * (long)(-timezone_sign()))
#endif
          );

  return finalstr;
}


/*
 *
 *
 *                                   External routines.
 *
 *
 */


/*  
 *  Construct a special information header containing a standard (RFC822)
 *  header, and send it, along with the output data, to a special port.
 */  

int mail_it(ac, av, old_cmd, mfp)
  int ac;
  char **av;
  const char *old_cmd;
  FILE *mfp;
{
  FILE *ofp;
  char buffer[INPUT_LINE_LEN];
  char hname[MAXHOSTNAMELEN+1];
  const char *compress;
  const char *encode;
  const char *maddr;
  const char *mail_service;
  int ofd;
  struct servent *servp;

  ptr_check(av, char *, curr_lang[153], 0);

  switch (ac)
  {
  case 1:
    if (is_set(V_MAILTO))
    {
      maddr = get_var(V_MAILTO);
    }
    else
    {
      printf(curr_lang[155]);
      return 0;
    }
    break;

  case 2:
    maddr = av[1];
    break;

  default:
    printf(curr_lang[156], av[0]);
    return 0;
  }

  if ( ! mfp || ! *old_cmd)
  {
    printf(curr_lang[157]);
    return 0;
  }
  
  /*  
   *  Check whether there is actually anything to mail.
   */
  rewind_fp(mfp);
  if (fempty(mfp))
  {
    printf("# There is no output to mail.\n"); /*FFF*/
    return 0;
  }
  
  if ( ! (compress = get_var(V_COMPRESS)))
  {
    compress = curr_lang[158];
  }
  if ( ! (encode = get_var(V_ENCODE)))
  {
    encode = curr_lang[158];
  }
  if (strcmp(compress, curr_lang[158]) != 0 && strcmp(encode, curr_lang[158]) == 0)
  {
    printf(curr_lang[159]);
    return 0;
  }

  mail_service = get_var(V_MAIL_SERVICE);
  if (isdigit(*mail_service))
  {
    int port = atoi(mail_service);

    if (port == 0)
    {
      error(A_ERR, curr_lang[153], curr_lang[160], mail_service);
      return 0;
    }
    if ( ! (servp = getservbyport((int)htonl((unsigned long)port), (char *)0)))
    {
      error(A_INTERR, curr_lang[153], curr_lang[161], mail_service);
      return 0;
    }
  }
  else
  {
    if ( ! (servp = getservbyname(mail_service, (char *)0)))
    {
      error(A_INTERR, curr_lang[153], curr_lang[161], mail_service);
      return 0;
    }
  }

  get_archie_hostname(hname, sizeof hname);

  /* bug: does cliconnect do dotted decimal? */
  if (cliconnect(get_var(V_MAIL_HOST), servp->s_port, &ofd) != A_OK) /*bug: fix defn.*/
  {
    error(A_INTERR, curr_lang[153], curr_lang[162], get_var(V_MAIL_HOST));
    return 0;
  }

  if ( ! (ofp = fdopen(ofd, curr_lang[92])))
  {
    error(A_ERR, curr_lang[153], curr_lang[163]);
    close(ofd);
    return 0;
  }

  /*
    Create the informational header.
  */

  fprintf(ofp, curr_lang[164]);
  fprintf(ofp, curr_lang[165], old_cmd);
  fprintf(ofp, curr_lang[166], compress);
  fprintf(ofp, curr_lang[167], encode);
  fprintf(ofp, curr_lang[168], atoi(get_var(V_MAX_SPLIT_SIZE)));
  fprintf(ofp, curr_lang[169]);

  /*
    Create the mail header.
  */

  fprintf(ofp, curr_lang[170], maddr);
  fprintf(ofp, curr_lang[171], get_var(V_MAIL_FROM), hname);
  fprintf(ofp, curr_lang[172], get_var(V_MAIL_FROM), hname);
  fprintf(ofp, curr_lang[173], std_mail_time());
  fprintf(ofp, curr_lang[326]);  

/*  to disable  the vacation program by setting Precedence: junk in the 
    header of the mail  950807*/

  fprintf(ofp, curr_lang[174]);

  /* snarf 'n barf the results */

  while (fgets(buffer, sizeof buffer, mfp))
  {
    if (fputs(buffer, ofp) == EOF)
    {
      error(A_SYSERR, curr_lang[153], curr_lang[196]);
      fclose(ofp);
      return 0;
    }
  }
  fclose(ofp);
  if (ferror(mfp))
  {
    error(A_SYSERR, curr_lang[153], curr_lang[197]);
    return 0;
  }

  return 1;
}
