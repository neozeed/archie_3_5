#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "get-info.h"
#include "protos.h"


#define IS_DHEX(n) ((n) >= '0' && (n) <= '9')
#define IS_LHEX(n) ((n) >= 'a' && (n) <= 'f')
#define IS_UHEX(n) ((n) >= 'A' && (n) <= 'F')

/*
 *  HEX() assumes `n' is a valid hex digit #define HEX(n) (IS_DHEX(n) ? (n) -
 *  '0' : IS_LHEX(n) ? (n) - 'a' + 10 : (n) - 'A' + 10) #define IS_HEX(n)
 *  (IS_DHEX(n) || IS_LHEX(n) || IS_UHEX(n)) #define QCHAR '%' #define QSTRSZ
 *  3 * Quoted character string size (%hh) *
 */

extern char *prog;


/*
 *  Convert a hexadecimal digit to the equivalent binary value.
 *  
 *  bug: assumes ASCII.
 */
#define HEX(c) ((c) <= '9' ? (c) - '0' : (c) <= 'F' ? (c) + 10 - 'A' : (c) + 10 - 'a')

/*
 *  Return 1 if `c' is a valid hexadecimal digit, 0 otherwise.
 */
#define IS_HEX(c) (((c) >= '0' && (c) <= '9') ||        \
                   ((c) >= 'a' && (c) <= 'f') ||        \
                   ((c) >= 'A' && (c) <= 'F'))

/*
 *  The `quote' character that appears before two hexadecimal digits.
 */
#define QCHAR '%'

/*
 *  A table of 256 elements.  A 1 indicates the corresponding character should
 *  be quoted, 0 means the character may stand for itself.
 *  
 *  I have included the '+' charachter (43) to be quoted for the special
 *  case of strhan handle quoting and dequoting.
 */
static char quote[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

extern void getword(char *word, char *line, char stop) {
  int x = 0,y;

  for(x=0;((line[x]) && (line[x] != stop));x++)
	{	if ( line[x]!='\n' )
    word[x] = line[x];
	}
  word[x] = '\0';
  if(line[x]) ++x;
  y=0;

  while( (line[y++] = line[x++]) );
}

extern char *makeword(char *line, char stop) {
  int x = 0,y;
  char *word = (char *) malloc(sizeof(char) * (strlen(line) + 1));

  for(x=0;((line[x]) && (line[x] != stop));x++)
	{
    word[x] = line[x];
	}

  word[x] = '\0';
  if(line[x]) ++x;
  y=0;

  while(line[y])
	{
		if( line[x]=='\0' )
		{	line[y]='\0';}
		else if ( (line[x]) !=13 )
		{
			if ( isspace( line[x] ) && (line[x]!=32) && (line[x]>=0x00) )
			{	line[y++] = 32;
				x++;
			}else{
        line[y++] = line[x++];
      }
      /*	printf("-%d%c ",line[x],line[x]); */
		}
		else if ((line[x+1])&&(line[x+1]==10))
		{
      /*	printf("HEREHERE***%c %c\n",line[x],line[x+1]); */
			line[y++] = 32;
			x=x+2;
		}
		else
		{                           /*line[y++] = 32;*/
			++x;
		}
	}
  
  return word;
}

extern char *fmakeword(FILE *f, char stop, char non_stop, int *cl) {
  int wsize;
  char *word,next;
  int ll;

  next = '\0';
  wsize = 102400;
  ll=0;
  word = (char *) malloc(sizeof(char) * (wsize + 1));

  while(1) {
    word[ll] = (char)fgetc(f);
    next = (char)fgetc(f);
    if( ungetc(next,f) != next ){
      fprintf(stdout,"could not get word\n");
    }
    /*	printf("%c",word[ll]);
        printf("%d ",word[ll]);
        */
    if(ll==wsize) {
      word[ll+1] = '\0';
      wsize+=102400;
      word = (char *)realloc(word,sizeof(char)*(wsize+1));
    }
    --(*cl);
    if(((word[ll] == stop)&&(next!=non_stop)) || (feof(f)) || (!(*cl))) {
      if(word[ll] != stop) ll++;
      word[ll] = '\0';
      return word;
    }
    ++ll;
  }
}

extern char x2c(char *what) {
  register char digit;

  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
  return(digit);
}

int ind(char *s, char c) {
  register int x;

  for(x=0;s[x];x++)
  if(s[x] == c) return x;

  return -1;
}

#define c2x(what,where) sprintf(where,"%%%2x",what)

void escape_url(char *url) {
  register int x,y;
  char *copy;

  copy = strdup(url);

  for(x=0,y=0;copy[x];x++,y++) {
    if(ind("% ?+&",url[y] = copy[x]) != -1) {
      c2x(copy[x],&url[y]);
      y+=2;
    }
  }
  url[y] = '\0';
  free(copy);
}
  

extern void unescape_url(char *url) {
  register int x,y;

  for(x=0,y=0;url[y];++x,++y) {
    if((url[x] = url[y]) == '%') {
      url[x] = x2c(&url[y+1]);
      y+=2;
    }
  }
  url[x] = '\0';
}

extern void plustospace(char *str) {
  register int x;

  for(x=0;str[x];x++) if(str[x] == '+') str[x] = ' ';
}

extern int rind(char *s, char c) {
  register int x;
  for(x=strlen(s) - 1;x != -1; x--)
  if(s[x] == c) return x;
  return -1;
}

extern int getline(char *s, int n, FILE *f) {
  register int i=0;

  while(1) {
    s[i] = (char)fgetc(f);

    if(s[i] == CR)
    s[i] = fgetc(f);

    if((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) {
      s[i] = '\0';
      return (feof(f) ? 1 : 0);
    }
    ++i;
  }
}

extern int checkvalue(char *val)
{
	int i=0;
	int size;
	size = strlen(val);
	if((size>=MINIMUM_NM_SIZE)&&(size<=MAXIMUM_NM_SIZE))
	{	while( i<size )
		{	if(!isalnum(val[i]) )
      return(0);
    else if( islower(val[i]) )
    val[i] = toupper( val[i] );	
			++i;
		}
		return(1);
	}else	return(0);
}

extern void send_fd(FILE *f, FILE *fd)
{
  char c;

  while (1) {
    c = fgetc(f);
    if(feof(f))
    return;
    fputc(c,fd);
  }
}


/*
 * -The following was provided by Bill Heelan:
 *
 *  Routines to quote and dequote strings according to the rules of RFC 1738
 *  (Uniform Resource Locators).
 *  
 *  When quoted, a single character is represented as a three character
 *  sequence: a percent followed by two hexadecimal digits.  The two digits
 *  represent the ASCII code of the character.
 *  
 *  The characters that must be quoted include (in base 16) ASCII 00 through
 *  1F, 7F, and 80 through FF.  Other characters that should be quoted are:
 *  SPACE, `<' and `>', `"', `#', `%', `{', `}', `|', `\', `^', `~', `[', `]',
 *  and ``'.  As well, the following characters may sometimes have a special
 *  meaning, and should be quoted: `;', `/', `?', `:', `@', `=` and `&'.
 *  
 *  Alternatively, the only characters that may be safely left unquoted are:
 *  <alphanumerics> and `$', `-', `_', `.', `+', `!', `*', `'', `(', `)' and
 *  `,'.
 */

/*
 *  Return the length of the dequoted version of `s'.  Return -1 if `s'
 *  contains a malformed string (i.e. it contains a percent that is not
 *  followed by two hexadecimal digits).
 */
static long size_dequoted(const char *s)
{
  long sz = 0;

  while (*s) {
    if (*s != QCHAR) {
      sz++; s++;
    } else if (IS_HEX(s[1]) && IS_HEX(s[2])) {
      sz++; s += 3;
    } else {
      return -1;
    }
  }

  return sz;
}

/*
 *  Return the length of the quoted version of `s'.
 */
static long size_quoted(const char *s)
{
  long sz = 0;
  
  while (*s) {
    if (quote[(unsigned char)*s++]) {
      sz += 3;
    } else {
      sz++;
    }
  }

  return sz;
}

/*
 *  Return a pointer to malloc()ed memory containing a dequoted version of
 *  `src'.  The null pointer is returned if an error occurs.  (I.e. not enough
 *  memory could be allocated, or `src' contained a malformed string.)
 */
char *dequoteString(const char *src)
{
  char *d, *dstr;
  const char *s;
  long dsz;

  if ((dsz = size_dequoted(src)) < 0) {
    error(A_ERR,"dequoteString","malformed quoted string.\n");
    return 0;
  }

  if ( ! (dstr = malloc(dsz + 1))) {
    error(A_ERR,"dequoteString","can't allocate %lu bytes for dequoted string.\n",
           (unsigned long)dsz); perror("malloc");
    return 0;
  }

  s = src; d = dstr;
  while (*s) {
    if (*s != QCHAR) {
      *d++ = *s++;
    } else {
      if (s[1] == '2' && s[2] == '5' && s[3] != QCHAR ){
        s += 2;
      }
      *d++ = (HEX(s[1]) * 16) + HEX(s[2]); s += 3;
    }
  }

  *d = '\0';

  return dstr;
}

/*
 *  Return a pointer to malloc()ed memory containing a quoted version of
 *  `src'.  The null pointer is returned if not enough memory could be
 *  allocated to store the result.
 */
char *quoteString(const char *src)
{
  char *q, *qstr;
  const char *s;
  long qsz;

  qsz = size_quoted(src);

  if ( ! (qstr = malloc(qsz + 1))) {
    error(A_ERR,"quoteString","Can't allocate %lu bytes for quoted string.\n",
            (unsigned long)qsz); perror("malloc");
    return 0;
  }

  s = src; q = qstr;
  while (*s && qsz>0) {
    if (quote[(unsigned char)*s]) {
      sprintf(q, "%c%02x", QCHAR, (unsigned char)*s); s++; q += 3;
    } else {
      *q++ = *s++;
    }
  }

  *q = '\0';

  return qstr;
}
