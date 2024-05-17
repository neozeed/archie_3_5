#include "macros.h"
#include "misc.h"


#define SPIN (current_mod() != M_EMAIL && is_set(V_STATUS))

#define MACHINE -100 /* listing output formats */
#define SILENT  -101
#define TERSE   -102
#define VERBOSE -103
#define URL     -104

static StrVal style_list[] =
{
  {"machine", MACHINE},
  {"silent", SILENT},
  {"terse", TERSE},
  {"verbose", VERBOSE},
  {"url", URL},

  {FRENCH("ordinateur"), MACHINE},
  {FRENCH("silencieux"), SILENT},
  {FRENCH("breve"), TERSE},
  {FRENCH("verbose"), VERBOSE},
  {FRENCH("url"), URL},

  {(const char *)0, 0}
};

