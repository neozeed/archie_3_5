#ifndef MACROS_H
#define MACROS_H

#define CNTRL(c) ((c) & ~0x40)
#define INTERNAL_ERROR 100	/* for functions of enum type */
#define LRTNC(c) ((c) | 0x40)
#define MAX_HOST_LEN	128
#define MAX_STRING_LEN	2048
#define MIN_TO_SEC(m) ((unsigned int)((m)*60))
#define WHITE_SPACE "\t "
#define is_dot_or_dotdot(s) (s[0] == '.' && (s[1] == '\0' || (s[1] == '.' && s[2] == '\0')))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define streq(a, b) (*a == *b && strcmp(a, b) == 0) /* a little speedup *//*bug: inc. string.h*/

#ifdef MULTI_LING
#  define FRENCH(s) s
#else
#  define FRENCH(s) ((const char *)0)
#endif

#endif
