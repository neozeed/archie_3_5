#ifndef UTILS_H
#define UTILS_H

#ifdef __STDC__

extern char *nuke_nl(char *s) ;
extern char *strend(char *s) ;
extern char *strsep(char **s, char *set) ;
extern char *tail(char *path) ;
extern int fprintf_path(FILE *fp, char **p, int n) ;

#else

extern char *nuke_nl(/* char *s */) ;
extern char *strend(/* char *s */) ;
extern char *strsep(/* char **s, char *set */) ;
extern char *tail(/* char *path */) ;
extern int fprintf_path(/* FILE *fp, char **p, int n */) ;

#endif
#endif
