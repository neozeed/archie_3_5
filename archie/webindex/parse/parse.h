
#ifndef PARSE_H
#define PARSE_H


status_t parse_file PROTO(( file_info_t *, file_info_t *, file_info_t *, int*, ip_addr_t, int));


extern status_t excerpt_extract PROTO((FILE*,FILE*));
extern status_t keyword_extract PROTO((FILE*,FILE*, struct arch_stridx_handle *));

extern status_t parse_rec PROTO(( FILE*, FILE*, FILE*, sub_header_t *, long, struct arch_stridx_handle *));

extern int stoplist_setup PROTO((void));
extern void get_tmpfiles PROTO((pathname_t, pathname_t, pathname_t, char *));




#endif
