#ifndef _NET_WEBINDEX_H_
#define _NET_WEBINDEX_H_

extern    status_t	send_webindex_site PROTO(( ip_addr_t, header_t *, hostdb_aux_t *, format_t, char *,  struct arch_stridx_handle *, date_time_t));
extern    status_t	send_webindex_excerpt PROTO(( ip_addr_t, header_t *, hostdb_aux_t *, format_t, char *,  struct arch_stridx_handle *, date_time_t));
extern	  bool_t	xdr_parser_entry_t PROTO(( XDR *, parser_entry_t *));
extern	  status_t	get_webindex_site PROTO((file_info_t *, file_info_t *, int ));
extern	  status_t	get_webindex_excerpt PROTO((file_info_t *, file_info_t *, int ));
extern	  status_t	copy_xdr_to_parser PROTO((XDR *, file_info_t *, index_t));
extern	  status_t	copy_parser_to_xdr PROTO((XDR *, file_info_t *, index_t, date_time_t, struct arch_stridx_handle *));
/* #warning I am not sure this is properly defined */
extern	  void		sig_handle PROTO((int/*, int, struct sigcontext *, char * */));
extern void output_excerpt PROTO((file_info_t*,file_info_t*,FILE*,date_time_t,int));
#endif

