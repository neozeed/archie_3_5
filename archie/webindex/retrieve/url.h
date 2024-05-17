/* Definitions of a URL */

enum URLMethod { NONE, HTTP };

typedef struct _URL {
  char *method;       /* The access method */
  char *server;	      /* The host */
  char *port;		      /* The port number */
  char *local_url;    /* The local url */
  char *curl;		      /* Canonical url */
} URL;

#define METHOD_URL(url)	   ((url)->method)
#define SERVER_URL(url)	   ((url)->server)
#define PORT_URL(url)	     ((url)->port)
#define LOCAL_URL(url)	   ((url)->local_url)
#define CANONICAL_URL(url) ((url)->curl)


/* Function prototypes */
extern URL *urlParse PROTO(( char *urlStr));
extern URL *urlBuild PROTO(( char *method, char *server, char *port, char *local_url));
extern int urlRestricted PROTO(( URL *url, char **rurls, char *path));
extern int urlFree PROTO(( URL *url));
extern int urlClean PROTO(( URL *url));
extern int urlLocal PROTO((char *server, char *port, URL *url));
extern char *urlStrBuild PROTO((URL *url));


