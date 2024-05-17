#ifndef PPC_FRONT_END_H
#define PPC_FRONT_END_H

#ifndef TIMING

#define ppc_pget_at pget_at
#define ppc_m_get_menu m_get_menu
#define ppc_my_rd_vlink my_rd_vlink
#define ppc_m_item_description m_item_description
#define ppc_m_class m_class
#define ppc_p_get_dir p_get_dir
#define ppc_rd_vdir rd_vdir
#define ppc_vfsetenv vfsetenv
#define ppc_p_initialize p_initialize

#else

#include "prosp.h"


extern PATTRIB ppc_pget_at(VLINK link, const char *atname);
extern VLINK ppc_m_get_menu(VLINK link);
extern VLINK ppc_my_rd_vlink(const char *path);
extern char *ppc_m_item_description(VLINK v);
extern int ppc_m_class(VLINK v);
extern int ppc_p_get_dir(VLINK dlink, const char *components, VDIR dir, long flags, TOKEN *acomp);
extern int ppc_rd_vdir(const char *dirarg, const char *comparg, VDIR dir, long flags);
extern int ppc_vfsetenv(char *host, char *desc, char *x);
extern void ppc_p_initialize(char *a, int i, struct p_initialize_st *p);

#endif
#endif
