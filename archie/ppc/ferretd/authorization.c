#ifdef BUNYIP_AUTHENTICATION
#include <stdio.h>
#include <malloc.h>
#include <pwd.h>
#include <time.h>

#include "archie_strings.h" /* for str_sep() */
#include "authorization.h"
#include "protos.h"


#define CLASS_SEPARATOR ')'
#define MY_PASSWD_FILE 1

#define PASSWORD_FILE "etc/passwd" /* Grabs it from ~archie always for now */


extern char *passfile;


#ifdef MY_PASSWD_FILE 

/* THIS IS FOR NOW ONLY .... MUST BE CHANGED .... */
#define setpwfile my_setpwfile
#define endpwent my_endpwent
#define getpwent  my_getpwent

extern int atoi();

static FILE *fpasswd = NULL;
static struct passwd private_passwd;

static char passwd_buff[500];

static struct passwd *my_getpwent()
{
  char *tmp;
  static struct passwd_ptrs { 
    char *pw_name;
    char *pw_passwd;
    char *pw_uid;
    char *pw_gid;
    char *pw_age;
    char *pw_comment;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
  } private_ptrs;

  if (fgets(passwd_buff, 500, fpasswd) == NULL)
  {
    return NULL;
  }

  /* Find the different fields */

  private_ptrs.pw_name = passwd_buff;
  tmp = passwd_buff;
  while (*tmp != ':') tmp++;   /* Look for the passwd field */
  *tmp++ = '\0';
  private_ptrs.pw_passwd = tmp;
  while (*tmp != ':') tmp++;   /* Look for the pw_uid field */
  *tmp++ = '\0';
  private_ptrs.pw_uid = tmp;
  while (*tmp != ':') tmp++;   /* Look for the pw_gid field */
  *tmp++ = '\0';
  private_ptrs.pw_gid = tmp;
  while (*tmp != ':') tmp++;   /* Look for the pw_age field */
  *tmp++ = '\0';
  private_ptrs.pw_gecos = tmp;
  while (*tmp != ':') tmp++;   /* Look for the pw_dir field */
  *tmp++ = '\0';
  private_ptrs.pw_dir = tmp;
  while (*tmp != ':') tmp++;   /* Look for the pw_shell field */
  *tmp++ = '\0';
  private_ptrs.pw_shell = tmp;
  while (*tmp != '\n') tmp++;   
  *tmp++ = '\0';

  private_passwd.pw_name = (*private_ptrs.pw_name == '\0') ? NULL : private_ptrs.pw_name;
  private_passwd.pw_passwd = (*private_ptrs.pw_passwd == '\0') ? NULL : private_ptrs.pw_passwd;
  private_passwd.pw_uid = (*private_ptrs.pw_uid == '\0') ? -1 : atoi(private_ptrs.pw_uid);
  private_passwd.pw_gid = (*private_ptrs.pw_gid == '\0') ? -1 : atoi(private_ptrs.pw_gid);
  private_passwd.pw_age = NULL;
  private_passwd.pw_comment = NULL;
  private_passwd.pw_gecos = (*private_ptrs.pw_gecos == '\0') ? NULL : private_ptrs.pw_gecos;
  private_passwd.pw_dir = (*private_ptrs.pw_dir == '\0') ? NULL : private_ptrs.pw_dir;
  private_passwd.pw_shell = (*private_ptrs.pw_shell == '\0') ? NULL : private_ptrs.pw_shell;
 
  return &private_passwd;
}

static int my_setpwfile(name)
  char *name;
{

   char filename[1000];
   struct passwd *ptr;

   if ( *name != '/' ) {
     fpasswd = fopen("/etc/passwd","r");
     if (fpasswd == NULL ) {
	fprintf(stderr,"Cannot open password file %s\n","/etc/passwd");
	return 0;
     }
     while ( (ptr = getpwent()) != NULL ) {
	if ( strcmp("archie",ptr->pw_name) == 0 ) {
	  break;
	}
     }
     fclose(fpasswd);
     if ( ptr == NULL ) {
       fprintf(stderr,"Cannot find user archie in /etc/passwd\n");
       return 0;
     }


     sprintf(filename,"%s/%s",ptr->pw_dir,name);
   }
   else {
     strcpy(filename,name);
   }

   fpasswd = fopen(filename,"r");
   if (fpasswd == NULL ) {
      fprintf(stderr,"Cannot open password file %s\n",filename);
      return 0;
   }
   return 1;
}

static void my_endpwent()
{
  if (fpasswd != NULL) {
    fclose(fpasswd);
    fpasswd = NULL;
  }
}
#endif


static char DecodeVector[256];


static void init_DecodeVector()
{
  char c;
  char i = 0;
  int j = 0;
  static int already_init = 0;

  if (already_init)
  {
    return;
  }

  for (j = 0; j < 256; j++)
  {
    DecodeVector[j] = (char)0;
  }

  i = 0;
  for (c = 'A'; c <= 'Z'; c++,i++)
  {
    DecodeVector[(int)c] = i;
  }

  for (c = 'a'; c <= 'z'; c++,i++)
  {
    DecodeVector[(int)c] = i;
  }

  for (c = '0'; c <= '9'; c++,i++)
  {
    DecodeVector[(int)c] = i;
  }

  DecodeVector[(int)'+'] = (char)62;
  DecodeVector[(int)'/'] = (char)63;
  already_init = 1;
}


#define DECODE(A) (DecodeVector[(int)(A)] & 0x3f)

static char *decode(s)
  char *s;
{
  char *t,*head;

  init_DecodeVector();

  head = t = (char*)malloc(sizeof(char)*strlen(s));
  while (*s != '\0') {
    *t     = DECODE(*s) << 2 | DECODE(*(s+1)) >> 4;
    *(t+1) = DECODE(*(s+1)) << 4 | DECODE(*(s+2)) >> 2;
    *(t+2) = DECODE(*(s+2)) << 6 | DECODE(*(s+3));

    t +=3; s+=4;
  }
  *t = '\0';
  return head;
}


static void clean_tuples(class,date)
  char **class,**date;
{
  char *c;

  /* remove leading blanks */
  while (**class == '('  || **class == ' ' || **class == '\t')
  {
    (*class)++;
  }

  if (*date != NULL) {

    /* remove leading blanks */

    while (**date == '('  || **date == ' ' || **date == '\t')
    {
      (*date)++;
    }
    for (c = *date; *c != '\0' ;c++) {
      if (*c == ')') {
        *c = '\0';
      }
    }
  }
}

static int verify_dates(d)
  char *d;
{
  int date,year,day,month;
  time_t cal;
  struct tm *local;

  if (d == NULL || d[0] == '\0')
  {
    return 1;
  }

  date = atoi(d);               /* Format is YYMMDD or YYYYMMDD */

  if (date == 0)
  {
    return 1;
  }

  day = date%100;
  month = ((date-day)/100) % 100;
  year = (date-(month*100)+day) / 10000;

  if (year > 100)
  {
    year -= 1900;
  }
   
  if (time(&cal) == -1)
  {
    return 0;
  }

  local = localtime(&cal);
  if (local == NULL)
  {
    return 0;
  }

  if (year <  local->tm_year)
  {
    return 0;
  }
  else
  {
    if (year == local->tm_year)
    {
      if (month <  local->tm_mon)
      {
        return 0;
      }
      else
      {
        if (month == local->tm_mon)
        {
          if (day < local->tm_mday)
          {
            return 0;
          }
        }
      }
    }
  }

  return 1;
}


static int verify_classes(class_str,p)
  char *class_str;
  PATTRIB p;
{
  int i, j;
  char *pval;
  char **aptr, *class, *date;

  if (nth_token_str(p->value.sequence,0) == NULL) /* No classes then open to all */
  {
    return 1;
  }

  if (class_str == NULL || *class_str == '\0')
  {
    return 1;
  }

  /* The info is as follows [(class,[date]),]* */

  aptr = str_sep(class_str, ',');

  for (i=0; aptr[i] != NULL; i++)
  { continue; }

  if (i % 2) {
    fprintf(stderr, "Error .. tuples malformed in passwordfile\n");
    exit(1);
  }

  for (i=0; aptr[i] != NULL; i += 2) {
    if (aptr[i] == NULL || *aptr[i]=='\0') {
      fprintf(stderr, "Error .. tuples malformed in passwordfile\n");
      exit(1);
    }
    class = aptr[i];
    date = aptr[i+1];
    clean_tuples(&class, &date);

    for(j = 0; (pval = nth_token_str(p->value.sequence,j)) != NULL; j++) {
      if (strcmp(pval,class) == 0 && verify_dates(date)) {
        if (aptr)
        {
          free_opts(aptr);
        }
        return 1;
      }
    }
  }

  if (aptr)
  {
    free_opts(aptr);
  }

  return 0;
}


static int verify_user(u, p, c)
  char *u,*p;
  PATTRIB c;
{
  struct passwd *ptr;

  if (passfile == NULL)
  {
    passfile = PASSWORD_FILE;
  }

  if (setpwfile(passfile) == 0)
  {
    return 0;
  }

  while ((ptr = getpwent()) != NULL) {
    if (strcmp(u,ptr->pw_name) == 0) {
      return (strcmp(p,ptr->pw_passwd) == 0 && verify_dates(ptr->pw_dir) &&
              verify_classes(ptr->pw_shell,c));
    }
  }
  endpwent();
  return 0;
}


static int authorized(s,e,c)
  char *s; /* scheme */
  char *e; /* encoded user:passw string */
  PATTRIB c; /* classes */
{
  char *u;                      /* username */
  char *p;                      /* password */
  char *up;                     /* username:password entry */

  if (strcmp(s,"Basic")) {
    fprintf(stderr, "Basic scheme only supported \n");
    return 0;
  }
   
  up = (char*)decode(e);

  /* Username */
  u = up;
  while (*up != '\0' && *up != ':')
  {
    up++;
  }
  *up++ = '\0';
  p = up;

  return verify_user(u, p, c);
}


int parseAuthorization(t, p)
  char *t;
  PATTRIB p;
{
  char *s;                      /* scheme */
  char *e;                      /* encoded string */

  if (t == NULL) {
    if ((p = nextAttr(RESTRICTED_ACCESS,p)) == NULL)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  /* Find Scheme */
  while (*t != '\0' && (*t == ' ' || *t == '\t')) t++; /* skip leading white characters */
  s = t;
  while (*t != '\0' && *t != ' ' &&  *t != '\t') t++;
  *t++ = '\0';

  /* Encoding string */
  while (*t != '\0' && (*t == ' ' || *t == '\t')) t++; /* skip leading white characters */
  e = t;
  while (*t != '\0' && *t != ' ' && *t != '\t') t++;
  *t++ = '\0';

  return authorized(s, e, p);
}
#endif
