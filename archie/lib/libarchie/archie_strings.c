#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <varargs.h>
#include "archie_strings.h"
#include "error.h"
#include "lang_libarchie.h"

#include "protos.h"


/*  
 *  Split a string into substrings, based on a set of separator characters.
 *  
 *  Separator characters appearing at the beginning or end of the source
 *  string will result in leading or trailing empty substrings.  Also, a
 *  sequence of N separator characters appearing in the middle of the source
 *  string will result in N-1 empty substrings.
 *
 *  E.g.  `/foo///bar/' becomes `', `foo', `', `', `bar', `'.
 *  
 *  The return value is a pointer to a null terminated array of pointers to
 *  the substrings.  Only one block of memory is allocated, so only the
 *  returned pointer need be freed.
 *  
 *  Zero is returned on an error.
 */  
int strsplit(src, splchs, ac, av)
  const char *src;
  const char *splchs;
  int *ac;
  char ***av;
{
  char *mem;
  char **ptrmem;
  char *strmem;
  char *p, *s;
  int n;
  int memsz;
  int numss;
  int srclen;
  int ptrsz;

  /*  
   *  Determine the number of substring pointers we'll need.
   */  
  for (numss = 0, p = (char *)src; *p; p++)
  {
    if (strchr(splchs, *p)) numss++;
  }
  numss++;

  srclen = strlen(src);
  ptrsz = numss * sizeof (char *);
  memsz = ptrsz + srclen + 1;

  if ( ! (mem = malloc(memsz)))
  {
    return 0;
  }

  ptrmem = (char **)mem;
  strmem = mem + ptrsz;

  memcpy(strmem, src, srclen + 1);

  for (n = 0, s = p = strmem; *p; p++)
  {
    if (strchr(splchs, *p))
    {
      *p = '\0';
      ptrmem[n++] = s;
      s = p + 1;
    }
  }
  ptrmem[n] = s;

  *ac = numss;
  *av = ptrmem;

  return 1;
}


#define WHITESPACE " \t"


int splitwhite(src, ac, av)
  const char *src;
  int *ac;
  char ***av;
{
  char **argv;                  /* resulting array of string pointers */
  char *sptr;                   /* pointer to start of current result string */
  int nstrs;                    /* total number of strings */
  int strslen = 0;              /* total length of all strings, not including nuls */

  *ac = 0; *av = 0;

  /*  
   *  Count the number of strings, and the sum of their lengths.
   */  

  {
    int n = 0;
    const char *s = src;
    
    while (1) {
      const char *e;
      int len;
  
      s += strspn(s, WHITESPACE); /* skip leading white space */
      e = s + strcspn(s, WHITESPACE); /* skip over word */
      len = e - s;

      if (len == 0) {
        break;
      }

      s = e;
      n++; strslen += len;
    }

    nstrs = n;
  }
  
  if (nstrs == 0) {
    return 1;
  }

  /*  
   *  Allocate enough memory for the strings (including nul terminators) and
   *  the pointers to them.
   */

  if ( ! (argv = (char **)malloc(nstrs * sizeof(char **) + (strslen + nstrs)))) {
    return 0;
  }

  sptr = (char *)(argv + nstrs);

  /*  
   *  Reparse the source string, copying the words into the result, and
   *  setting up pointers to them.
   */

  {
    int n = 0;
    const char *s = src;

    strslen = 0;
    while (1) {
      const char *e;
      int len;

      s += strspn(s, WHITESPACE); /* skip leading white space */
      e = s + strcspn(s, WHITESPACE); /* skip over word */
      len = e - s;

      if (len == 0) {
        break;
      }

      argv[n] = sptr + strslen;
      strncpy(argv[n], s, len); *(argv[n] + len) = '\0';
      strslen++;
    
      s = e;
      n++; strslen += len;
    }
  }

  *ac = nstrs;
  *av = argv;

  return 1;
}


#define BAJAN_COMPAT 1

/*
 * strrcmp: analogous to strcmp(3) but in "reverse".  Compare strings
 * right-to-left. Same return values as strcmp(3)
 */


int strrcmp(key, element)
   char *key;	     /* strings to be compared */
   char *element;

{
   int key_len, element_len;
   register char *key_index, *element_index;

   if((key == (char *) NULL) || (element == (char *) NULL))
     return(0);

   key_len = strlen(key);
   element_len = strlen(element);

   /* point to the end of the strings, ignoring terminal NULL */

   key_index = key + key_len - 1;
   element_index = element + element_len - 1;

   /* do char by char comparison */

   for(; (key_index != key - 1) && (element_index != element - 1); key_index--, element_index--)
       if(*key_index != *element_index)
          return(*key_index - *element_index);

   if(key_index == key)
     return(-1);
   else
     return(0);

}

/*
 * strrcmp: analogous to strncmp(3) but in "reverse".  Compare strings
 * right-to-left. Same return values as strcmp(3)
 */


int strrncmp(key, element, size)
   char *key;	     /* strings to be compared */
   char *element;
   int size;

{
   int key_len, element_len;
   register char *key_index, *element_index;
   int i;

   if((key == (char *) NULL) || (element == (char *) NULL))
     return(0);

   key_len = strlen(key);
   element_len = strlen(element);

   /* point to the end of the strings, ignoring terminal NULL */

   key_index = key + key_len - 1;
   element_index = element + element_len - 1;

   /* do char by char comparison */

   for(i = 0; (i != size) && (key_index != key - 1) && (element_index != element - 1); key_index--, element_index--)
       if(*key_index != *element_index)
          return(*key_index - *element_index);

   if(i == size)
      return(0);

   if(key_index == key)
     return(-1);
   else
     return(0);

}


/*
 * make_lcase: convert the given string to lowercase characters.  modifies
 * argument. Also returns pointer to result
 */

char *make_lcase( string )
	char *string;

{

   char *c;

   if(string == (char *) NULL)
      return((char *) NULL);
      
   for(c = string; *c != '\0'; c++ ){
      if(isascii((int)*c) && isalpha((int)*c) && isupper((int)*c))
        *c = (char)tolower(*c);
   }

   return(string);
}


/*
 * str_decompose: break up a string composed of tokens separated by a
 * delimiter into the given variables. Used when number of tokens is known
 * in advance. Original string is not modified. If appropriate number of
 * arguments are not given, unpredictable results may occurr.
 *
 * Called as: str_decompose(string, delimit, arg [,arg...]);
 *
 * Uses a variable argument list.
 */
   

status_t str_decompose(va_alist)
   va_dcl
{
   va_list al;	     /* Original varargs list */
   char  delim;	     /* delimiter */
   char  *input_str; /* input string */
   char	 *input_cp;
   char  *curr_arg;
   char  *begin_ptr; /* Start of input string */
   char  *end_ptr;   /* End of input string */
   char  *curr_ptr;



   va_start(al);

   input_str = va_arg(al, char *);

   /* char's are promoted to int on procedure call */

   delim = va_arg(al, int);

   if((input_cp = strdup(input_str)) == (char *) NULL){

      /* "Can't allocate space for string" */

      error(A_INTERR,"str_decompose", STR_DECOMPOSE_001);
      return(ERROR);
   }

   begin_ptr = input_cp;
   end_ptr = input_cp + strlen(input_cp) + 1;
   curr_ptr = strchr(input_cp, delim);

   if(curr_ptr != (char *) NULL){

      *curr_ptr = '\0';
      
      while(curr_ptr < end_ptr){

	 curr_arg = va_arg(al, char *);

	 if(curr_arg != (char *) NULL)
	    strcpy(curr_arg, begin_ptr);

	 begin_ptr = curr_ptr + 1;

	 curr_ptr = strchr(begin_ptr, delim);

	 if(curr_ptr == (char *) NULL)
	    break;

	 *curr_ptr = '\0';
      }
   }

   curr_arg = va_arg(al, char *);

   if(curr_arg != (char *) NULL)
      strcpy(curr_arg, begin_ptr);

   va_end(al);

   return(A_OK);
}

char *strndup(s, n)
  const char *s;
  int n;
{
  char *mem;
  
  if ((mem = malloc(n+1)))
  {
    strncpy(mem, s, n);
    mem[n] = '\0';
  }
  return mem;
}


/*  
 *  Break the string `src' into substrings, each of which is delimited by the
 *  character `delim'.  For example, if the delimiter is "/", then "a/b"
 *  would become [a,b], "/a" would become [,a], etc.
 *  
 *  There are two special cases. If `src' ends with a delimiter no final
 *  empty string is allocated.  That is, "a/" would result in [a], rather
 *  than [a,].  The second case is when `src' is an empty string, in which
 *  case the return value is [] (i.e. a one element array containing a null
 *  pointer).
 *  
 *  Return a null terminated array of pointers to the substrings, where each
 *  non-null element points to free()able memory.  The array itself is also
 *  free()able.
 *  
 *  A null pointer is returned upon error; no memory is leaked.
 */                    
char **str_sep(src, delim)
  const char *src;
  int delim;
{
#define try(x) \
  do { if ( ! (x)) goto fail; } while (0)
  
  char **ptrs, **res;
  const char *sc;               /* scan for delimiters */
  const char *st;               /* start of substring */
  int n = 0;

  /*  
   *  Check arguments.
   */  
  if ( ! src || delim == '\0')
  {
    error (A_INTERR, "str_sep", STR_SEP_001);
    return (char **)0;
  }
  
  /*  
   *  Check for an empty string (the second special case).
   */  
  if (src[0] == '\0')
  {
    if ( ! (res = (char **)malloc(sizeof (char *))))
    {
      error (A_INTERR, "str_sep", STR_SEP_001);
      return (char **)0;
    }
    *res = 0;
    return res;
  }
  
  /*  
   *  We will need at most `n+2' character pointers, where `n' is the number
   *  of delimiter characters.
   */  
  for (sc = src; *sc; sc++)
  {
    if (*sc == delim) n++;
  }

  if ( ! (res = ptrs = (char **)malloc((n + 2) * sizeof (char **))))
  {
    error (A_INTERR, "str_sep", STR_SEP_001);
    return (char **)0;
  }

  /*  
   *  Scan through the source string.  Stop at `delim' characters and
   *  strndup() the preceding string into the next location in the `ptrs'
   *  array.
   */    
  st = sc = src;
  while (*sc)
  {
    if (*sc != delim)
    {
      ++sc;
    }
    else
    {
      try(*ptrs++ = strndup(st, sc - st));
      st = ++sc;
    }
  }

  /*  
   *  The only time we _don't_ strdup() the final string is if the last
   *  character of the source string is a delimiter.  I.e. (st == sc) != src.
   */  
  if (st != sc || st == src)
  {
    try(*ptrs++ = strdup(st));
  }
  *ptrs = 0;
  return res;


 fail:
  /*  
   *  strndup() or strdup() failed: deallocate what we've created and return 0.
   */
  for (ptrs = res; *ptrs; ptrs++)
  {
    free(*ptrs);
  }
  free(res);
  error (A_INTERR, "str_sep", STR_SEP_001);
  return (char **)0;
  
#undef try
}


/*  Similar to str_sep, with the exception that only one malloc is called. 
 *  The tokenized strings are placed at the end the char ** array. 
 *
 *  Break the string `src' into substrings, each of which is delimited by the
 *  character `delim'.  For example, if the delimiter is "/", then "a/b"
 *  would become [a,b], "/a" would become [,a], etc.  The only special case
 *  occurs if `src' ends with a delimiter: no final empty string is
 *  allocated.  That is, "a/" would result in [a], rather than [a,].
 *
 *  A null pointer is returned upon error; no memory is leaked.
 */            
char **str_sep_single_free(src, delim)
  const char *src;
  int delim;
{
  char **ptrs;
  char **res;
  char *dst;
  int i;
  int n = 0;
  
  for (i = 0; src[i]; i++)
  {
    if (src[i] == delim) n++;
  }

  /* we need i + (n + 1 + 1) * sizeof(char *) bytes */
  if ( ! (res = ptrs = (char **)malloc(i + 1 + (n+1+1)*sizeof (char *))))
  {
    error (A_INTERR, "str_sep", STR_SEP_001);
    return (char **)0;
  }

#if 0
  printf("%d bytes for string; %d pointers (incl. term. null); %ld total.\n",
         i+1, n+2, i + (n+1+1)*sizeof(char *));
#endif

  dst = (char *)ptrs + (n+1+1) * sizeof (char *);
  *ptrs++ = dst;
  do
  {
    if (*src == delim)
    {
      *dst = 0;
#if BAJAN_COMPAT
      /*  
       *  So that `foo/' becomes (foo) rather than (foo,).
       *  
       *  (for compatibility with Alan's version of str_sep())
       */    
      if (*(src+1)) *ptrs++ = dst+1;
#else
      *ptrs++ = dst+1;
#endif
    }
    else
    {
      *dst = *src;
    }
    dst++;
  }
  while (*src++);
  *ptrs = 0;
  return res;
}

#if 0
void Dprtlist(list)
  char *list[];
{
  int i;
  
  for (i = 0; list[i]; i++)
  {
    printf("\t#%d [%s]\n", i, list[i]);
  }
}
#endif

#if 0  /* BAD_STR_SEP */

/* The initial default number of strings to be allocated */

#define DEF_NUM_STR   10

/*
 * str_sep: similar to str_decompose. Used when the number of tokens is not
 * known in advance. Returns a pointer to an array of char pointers which
 * point to the resulting tokens. List is terminated with a NULL pointer.
 * Returns (char **) NULL on error.
 */

char **str_sep(list, delimit)
  char *list;	     /* String containing tokens to be broken up */
  int  delimit;     /* character delimiting tokens */
{

  char *input_cp;
  char *begin_ptr;
  char *end_ptr;
  char *curr_ptr;
  char **output_ptr;
  int	 count = 0;
  int  max_count = DEF_NUM_STR;
  char delim = (char) delimit;

  if((output_ptr = (char **) malloc(max_count * sizeof(char *))) == (char **) NULL)
  {
    error(A_INTERR, "str_sep", STR_SEP_001);
    return((char **) NULL);
  }

  if((input_cp = strdup(list)) == (char *) NULL)
  {
    /* "Can't allocate space for string" */
    error(A_INTERR,"str_decompose", STR_SEP_001);
    return((char **) NULL);
  }

  begin_ptr = input_cp;
  end_ptr = input_cp + strlen(input_cp) + 1; /* set to one byte after nul */
  curr_ptr = strchr(input_cp, delim);

  if(curr_ptr != (char *) NULL)
  {
    *curr_ptr = '\0';
    while(curr_ptr < end_ptr)
    {
      output_ptr[count] = strdup(begin_ptr);
      begin_ptr = curr_ptr + 1;
      curr_ptr = strchr(begin_ptr, delim);
      if(curr_ptr == (char *) NULL)
      {
        break;
      }
      *curr_ptr = '\0';
      count++;
      if(count == max_count)
      {
        if((output_ptr = (char **) realloc(output_ptr, (max_count + DEF_NUM_STR) * sizeof(char *))) == (char **) NULL)
        {
          error(A_INTERR,"str_sep", STR_SEP_001);
          return((char **) NULL);
        }
        max_count += DEF_NUM_STR;
      }
    }

    count++;
    if(count+1 == max_count)
    {
      if((output_ptr = (char **) realloc(output_ptr, (max_count + DEF_NUM_STR) * sizeof(char *) )) == (char **) NULL)
      {
        error(A_INTERR,"str_sep", STR_SEP_001);
        return((char **) NULL);
      }
      max_count += DEF_NUM_STR;
    }
  }

#ifdef _DEBUG_MALLOC_INC
  malloc_chain_check(0);
#endif

  if(begin_ptr[0] != '\0')
  {
    output_ptr[count++] = strdup(begin_ptr);
  }
  output_ptr[count] = (char *) NULL;
  free(input_cp);
  return(output_ptr);
}

#endif /* BAD_STR_SEP */

/*
 * free_opts: Free the array of character pointers allocated by something
 * like str_sep.
 */

void free_opts(restricts)
   char **restricts;

{
   register char **res = restricts;;

   if(restricts == (char **) NULL)
      return;

   for(; (restricts != (char **) NULL) && (*restricts != (char *) NULL); restricts++)
      free(*restricts);

   free(res);
}
      
/*
 * insert_char: Insert a character into an array at the given position, moving
 * following characters down one place. This assumes that the input array
 * has enough space to insert the character 
 */


void insert_char(array, inchar, position)
   char *array;	  /* Array that character is to be inserted into */
   int inchar;	  /* character to be inserted */
   int  position; /* subscript of array at which the character is to be inserted */

{

   int len = strlen(array);

   if((array == (char *) NULL) || (position < 0))
      return;

   if(position == len){
      array[position] = (char) inchar;
      array[position + 1] = '\0';
      return;
   }

   /* bcopy must handle overlapping copies correctly !!! */
   /* remember the terminal NULL on strings */

#ifndef SOLARIS
   bcopy(array + position, array + position + 1, len - position + 1); 
#else
   memmove(array + position + 1, array + position, len - position + 1); 
#endif

   array[position] = (char) inchar;
   return;
}

/* delete_char: remove a character from the given subscript in the array */


void delete_char(array, inchar, position)
   char *array;	  /* array from which character is to be deleted */
   int inchar;	  /* ignored. Used for symmetry with insert_char */
   int  position; /* subscript at which character is to be deleted */

{

   int len = strlen(array);

   if((array == (char *) NULL) || (position < 0))
      return;

   if(position == len){
      array[position] = '\0';
      return;
   }

   /* bcopy must handle overlapping copies correctly !!! */
   /* remember the terminal NULL on strings */

#ifndef SOLARIS
   bcopy(array + position + 1 , array + position, len - position + 1);
#else
   memmove(array + position, array + position + 1, len - position + 1); 
#endif

   return;
}

/*
 * cmp_strcase_ptr: compare two arrays of pointers to strings
 */

int cmp_strcase_ptr(a, b)
   char **a;
   char **b;
{
   return(strcasecmp(*a, *b));
}

#if 0

int vslen(const char *fmt, va_list ap)
{
  static FILE *nfp = 0;
  int len;

  if ( ! nfp && ! (nfp = fopen("/dev/null", "w")))
  {
    error(A_ERR, "vslen", "can't open `/dev/null' for writing");
    return -1;
  }
  len = vfprintf(nfp, fmt, ap);
  return len;
}

#endif
