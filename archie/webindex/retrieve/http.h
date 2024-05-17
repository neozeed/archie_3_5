/* http.h */

typedef struct {
   struct tm *IfModSince;  /* The modified since time */
   char *uagent;	   /* The user agent */
   char *uaver;		   /* The user agent version */
   char *from;		   /* The from email addr */
   char *accept;     /* The Accept  line */
} HTTPReqHead;


/* Function prototypes */


extern int httpGetfp PROTO((URL *url, HTTPReqHead *param, int timeout, int *ret, FILE *html_fp, FILE *http_fp));
extern int httpHeadfp PROTO((URL *url, HTTPReqHead *param, int timeout, int *ret, FILE *html_fp, FILE *http_fp));
