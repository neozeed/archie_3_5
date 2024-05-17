#include <sys/time.h>
#include "error.h"
#include "misc.h"
#include "my_rd_vlink.h"
#include "p_menu.h"
#include "ppc_front_end.h"
#include "protos.h"
#include "prosp.h"
#include "all.h"


#ifdef TIMING

#define Dfprintf dfprintf

#define TIME(caller, call) \
do                                                                   \
{                                                                    \
 struct timeval end;                                                 \
 struct timeval start;                                               \
                                                                     \
 gettimeofday(&start, (struct timezone *)0);                         \
 call;                                                               \
 gettimeofday(&end, (struct timezone *)0);                           \
 fprintf(stderr, "%s: %s: elapsed time = %.3f milliseconds.\n",      \
         prog, caller,                                               \
         (end.tv_sec - start.tv_sec) * 1000.0 +                      \
         (end.tv_usec - start.tv_usec) / 1000.0);                    \
} while(0)



char *ppc_m_item_description(VLINK v)
{
  char *r;

  Dfprintf(stderr, "%s: ppc_m_item_description: `%s:%s'.\n",
           prog, v->host, v->hsoname);
  TIME("ppc_m_item_description", r = m_item_description(v));
  return r;
}


void ppc_p_initialize(char *a, int i, struct p_initialize_st *p)
{
  Dfprintf(stderr, "%s: ppc_p_initialize.\n", prog);
  TIME("ppc_p_initialize", p_initialize(a, i, p));
}


int ppc_vfsetenv(char *host, char *desc, char *x)
{
  int r;
  
  Dfprintf(stderr, "%s: ppc_vfsetenv.\n", prog);
  TIME("ppc_vfsetenv", r = vfsetenv(host, desc, x));
  return r;
}


int ppc_m_class(VLINK v)
{
  int r;

  Dfprintf(stderr, "%s: ppc_m_class: `%s:%s'.\n",
           prog, v->host, v->hsoname);
  TIME("ppc_m_class", r = m_class(v));
  return r;
}


int ppc_p_get_dir(VLINK dlink, const char *components, VDIR dir, long flags, TOKEN *acomp)
{
  int r;

  Dfprintf(stderr, "%s: ppc_p_get_dir: get `%s' in `%s:%s' (flags 0x%08lx).\n",
           prog, components, dlink->host, dlink->hsoname, flags);
  TIME("ppc_p_get_dir", r = p_get_dir(dlink, components, dir, flags, acomp));
  return r;
}


PATTRIB ppc_pget_at(VLINK link, const char *atname)
{
  PATTRIB r;

  Dfprintf(stderr, "%s: ppc_pget_at: get `%s' for `%s:%s'.\n",
           prog, atname, link->host, link->hsoname);
  TIME("ppc_pget_at", r = pget_at(link, atname));
  return r;
}


int ppc_rd_vdir(const char *dirarg, const char *comparg, VDIR dir, long flags)
{
  int r;

  Dfprintf(stderr, "%s: ppc_rd_vdir: get `%s' in `%s' (flags 0x%08lx).\n",
           prog, comparg, dirarg, flags);
  TIME("ppc_rd_vdir", r = rd_vdir(dirarg, comparg, dir, flags));
  return r;
}


VLINK ppc_m_get_menu(VLINK link)
{
  VLINK v;
  
  Dfprintf(stderr, "%s: ppc_m_get_menu: menu in `%s:%s'.\n",
           prog, link->host, link->hsoname);
  TIME("ppc_m_get_menu", v = m_get_menu(link));
  return v;
}


VLINK ppc_my_rd_vlink(const char *path)
{
  VLINK v;
  
  Dfprintf(stderr, "%s: ppc_my_rd_vlink: `%s'.\n",
           prog, path);
  TIME("ppc_my_rd_vlink", v = my_rd_vlink(path));
  return v;
}
#endif
