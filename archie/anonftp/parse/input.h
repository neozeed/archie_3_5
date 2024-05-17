#ifndef INPUT_H
#define INPUT_H

#ifdef __STDC__

extern int get_line(char *l, int len, FILE *fp) ;
extern int line_num(void) ;
extern void skip_line(void) ;

#else

extern int get_line(/* char *l, int len, FILE *fp */) ;
extern int line_num(/* void */) ;
extern void skip_line(/* void */) ;

#endif
#endif
