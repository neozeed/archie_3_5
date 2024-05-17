#include "macros.h"


static StrVal mode_strings[] =
{
  { "e-mail", M_EMAIL },
  { "interactive", M_INTERACTIVE },
  { "system rc", M_SYS_RC },
  { "user rc", M_USER_RC },

  { FRENCH("courrier \351lectronique"), M_EMAIL },
  { FRENCH("interactif"), M_INTERACTIVE },
  { FRENCH("initialisation syst\350me"), M_SYS_RC },
  { FRENCH("initialisation usager"), M_USER_RC },
  { (const char *)0, 0 }
};
