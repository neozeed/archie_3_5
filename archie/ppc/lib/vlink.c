#include <string.h>
#include "defs.h"
#include "error.h"
#include "misc.h"
#include "p_menu.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "vlink.h"
#include "all.h"

  
/*  
 *  Given the VLINK of a directory, look for the file `name' in it.
 */  
VLINK file_in_vdir(parent, name)
  VLINK parent;
  const char *name;
{
  VDIR rd;
  VDIR_ST rd_;
  rd = &rd_;
  vdir_init(rd);
  
  if (ppc_p_get_dir(parent, name, rd, GVD_ATTRIB|GVD_NOSORT, (TOKEN *)0) != PSUCCESS)
  {
    efprintf(stderr, "%s: file_in_vdir: p_get_dir() of `%s' in `%s:%s' failed: ",
             logpfx(), name, parent->host, parent->hsoname);
    perrmesg((char *)0, 0, (char *)0);
  }

  return rd->links;             /*bug? multiple files?*/
}


/*  
 *  `dir' should be a full pathname, with no tricky stuff (e.g. /../, ./,
 *  etc.)
 */  
int vcd(dir)
  const char *dir;
{
  VDIR d;
  VDIR_ST d_st;
  int r;
  int ret = 0;

  d = &d_st;
  vdir_init(d);
  if ((r = ppc_rd_vdir(dir, 0, d, RVD_DFILE_ONLY)) != PSUCCESS)
  {
    efprintf(stderr, "%s: vcd: error getting directory `%s': ", logpfx(), dir);
    perrmesg(0, r, 0);
  }
  else if ( ! d->links)
  {
    efprintf(stderr, "%s: vcd: `%s' not found.\n", logpfx(), dir);
  }
  else
  {
    pset_wd(d->links->host, d->links->hsoname, (char *)dir);
    ret = 1;
  }
  if (d->links) vllfree(d->links);
  if (d->ulinks) vllfree(d->ulinks);
  return ret;
}
