#ifndef VMS2_H
#define VMS2_H

#ifdef __STDC__

extern int v2db_type(char *v, core_site_entry_t *cse) ;
extern int v2db_time(char *v, char *x, core_site_entry_t *cse) ;
extern int v2db_size(char *v, core_site_entry_t *cse) ;
extern int v2db_perm(char *v, core_site_entry_t *cse) ;
extern int v2db_owner(char *v, core_site_entry_t *cse) ;
extern int v2db_name(char *v, core_site_entry_t *cse) ;
extern int v2db_links(char *v, core_site_entry_t *cse) ;
extern int v2db_date(char *v, core_site_entry_t *cse) ;

#else

extern int v2db_type(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_time(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_size(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_perm(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_owner(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_name(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_links(/* char *v, core_site_entry_t *cse */) ;
extern int v2db_date(/* char *v, core_site_entry_t *cse */) ;

#endif
#endif
