#include "error.h"
#include "lang.h"
#include "misc.h"
#include "mode.h"


#include "mode_lang.h"


static int mode = M_SYS_RC; /* the current mode */


int current_mode()
{
  return mode;
}


const char *mode_str(m)
  int m;
{
  const char *s;

  switch (m)
  {
  case M_SYS_RC:
  case M_USER_RC:
  case M_EMAIL:
  case M_INTERACTIVE:
    valtostr(m, &s, mode_strings);
    return s;

  case M_NONE:
    return curr_lang[205];

  default:
    return curr_lang[206];
  }
}


int set_mode(m)
  int m;
{
  switch (m)
  {
  case M_SYS_RC:
  case M_USER_RC:
  case M_EMAIL:
  case M_INTERACTIVE:
  case M_NONE:
    mode = m;
    break;

  default:
    mode = M_INTERACTIVE;
    error(A_INTERR, curr_lang[207], curr_lang[208], m, mode_str(mode));
    break;
  }

  return mode;
}
