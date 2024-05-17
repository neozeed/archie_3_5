#ifndef UNIX2_H
#define UNIX2_H

#ifdef __STDC__

extern int u2db_links(const char *v, core_site_entry_t *cse) ;
extern int u2db_name(const char *v, core_site_entry_t *cse) ;
extern int u2db_owner(const char *u, const char *v, core_site_entry_t *cse) ;
extern int u2db_perm(const char *v, core_site_entry_t *cse) ;
extern int u2db_size(const char *v, core_site_entry_t *cse) ;
extern int u2db_time(const char *mon, const char *day, const char *t_y, core_site_entry_t *cse) ;
extern int u2db_type(const char *v, core_site_entry_t *cse) ;

#else

extern int u2db_links(/* const char *v, core_site_entry_t *cse */) ;
extern int u2db_name(/* const char *v, core_site_entry_t *cse */) ;
extern int u2db_owner(/* const char *u, const char *v, core_site_entry_t *cse */) ;
extern int u2db_perm(/* const char *v, core_site_entry_t *cse */) ;
extern int u2db_size(/* const char *v, core_site_entry_t *cse */) ;
extern int u2db_time(/* const char *mon, const char *day, const char *t_y, core_site_entry_t *cse */) ;
extern int u2db_type(/* const char *v, core_site_entry_t *cse */) ;

#endif
#endif
