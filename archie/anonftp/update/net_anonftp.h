#ifndef _NET_ANONFTP_H_
#define _NET_ANONFTP_H_

extern    status_t	send_anonftp_site PROTO(( ip_addr_t, header_t *, hostdb_aux_t *, format_t, char *,  struct arch_stridx_handle *));
extern	  bool_t	xdr_parser_entry_t PROTO(( XDR *, parser_entry_t *));
extern	  status_t	get_anonftp_site PROTO((file_info_t *, file_info_t * ));
extern	  status_t	copy_xdr_to_parser PROTO((XDR *, file_info_t *, index_t));
extern	  status_t	copy_parser_to_xdr PROTO((XDR *, file_info_t *, index_t,    struct arch_stridx_handle *));
/* #warning I am not sure this is properly defined */
extern	  void		sig_handle PROTO((int/*, int, struct sigcontext *, char * */));

#endif

