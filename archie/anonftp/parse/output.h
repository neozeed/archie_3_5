#ifndef OUTPUT_H
#define OUTPUT_H

#define PAD 4

#ifdef __STDC__
extern int print_core_info(FILE *fp, parser_entry_t *pe, char *name) ;
#else
extern int print_core_info(/* FILE *fp, parser_entry_t *pe, char *name */) ;
#endif

#endif
