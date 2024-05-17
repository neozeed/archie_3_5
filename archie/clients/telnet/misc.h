#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "ansi_compat.h"
#include "prosp.h"


typedef struct
{
  const char *str;
  const int val;
} StrVal;


extern char *getProspMOTD PROTO((const char *server));
extern char *head PROTO((const char *path, char *buf, int bufsize));
extern char *squeeze_whitespace PROTO((char *str));
extern char *strndup PROTO((const char *s, int n));
extern char *strxlate PROTO((char *s, int fromc, int toc));
extern const char *attr_str PROTO((VLINK vl, const char *astr));
extern const char *get_home_dir_by_name PROTO((const char *name));
extern const char *get_home_dir_by_uid PROTO((int uid));
extern const char *now PROTO((void));
extern int bracketstr PROTO((const char *s, const char **start, const char **end));
extern int chroot_for_exec PROTO((const char *dir));
extern int dequote PROTO((char *qs));
extern int fempty PROTO((FILE *fp));
extern int fprint_file PROTO((FILE *ofp, const char *fname, int quietly));
extern int fprint_fp PROTO((FILE *ofp, FILE *ifp));
extern int fprintfProspMOTD PROTO((FILE *ofp, const char *server));
#ifndef P5_WHATIS
extern int initskip PROTO((const char *pattern, int len, int ignore_case));
#endif
extern int install_term PROTO((int ac, char **av));
extern int mode_truncate_fp PROTO((FILE *fp));
extern int prosp_strftime PROTO((char *buf, int bufsize, const char *fmt, const char *prosp_time));
extern int rewind_fp PROTO((FILE *fp));
extern int spin PROTO((void));
extern int swap_reuid PROTO((void));
extern int truncate_fp PROTO((FILE *fp));
#ifndef P5_WHATIS
extern int strfind PROTO((const char *txt, int tlen));
#endif
extern int strtoval PROTO((const char *str, int *val, StrVal *sv));
extern int valtostr PROTO((int val, const char **str, StrVal *sv));
extern int we_are_suid PROTO((void));
extern void dir_file PROTO((const char *path, char **dir, char **file));
extern void fmtprintf PROTO((FILE *ofp, const char *pfix, const char *sfix, int len));
extern void fprint_srch_restrictions PROTO((FILE *fp));
extern void freeProspMOTD PROTO((char *s));
extern void nuke_newline PROTO((char *line));

#endif
