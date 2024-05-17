/* This file is intended for inclusion by ardp or pfs */
/* NOTE: It is *not* dependent on pmachine.h */

/* Internal error handling routines used by the pfs code; formerly in */
/* internal_error.h.  These include a replacement for the assert()    */
/* macro, and an interface for internal error handling, better        */
/* documented in internal_error.c                                     */

#ifndef NDEBUG
  /* This is duplicated over in ardp.h.  A final cleanup will need to be made.
     */ 
#ifndef assert
#define assert(expr) do { \
    if (!(expr)) \
          p__finternal_error(__FILE__, __LINE__, "assertion violated: " #expr); \
} while(0)
#endif                          /* assert() */
#else /* NDEBUG */
#ifndef assert
#define assert(expr) do {;} while(0)
#endif /* assert() */
#endif /* NDEBUG */



#ifndef internal_error
#define internal_error(msg) do { \
      write(2, "Internal error in file " __FILE__ ": ", \
            sizeof "Internal error in file " __FILE__ ": " -1); \
      write(2, msg, strlen(msg)); \
      write(2, "\n", 1);        \
      if (internal_error_handler)   \
          (*internal_error_handler)(__FILE__, __LINE__, msg);   \
      else  { \
          fprintf(stderr, "line of error: %d\n", __LINE__); \
          abort(); \
      } \
  } while(0)
#endif /* internal_error */

/* This function may be set to handle internal errors.  Dirsrv handles them in
   this way, by logging to plog.  We make it int instead of void, because
   older versions of the PCC (Portable C Compiler) cannot handle pointers to
   void functions. */
extern int (*internal_error_handler)(const char file[], int linenumber, const char mesg[]);

/* function form of internal_error. */
void p__finternal_error(const char file[], int linenumber, const char mesg[]);
