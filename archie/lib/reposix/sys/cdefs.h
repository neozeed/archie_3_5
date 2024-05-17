/*
 *  A hack to get the regex code to compile.
 *  
 *  - wheelan
 */

#define __BEGIN_DECLS
#define __END_DECLS

#if __STDC__ == 1
# define __P(x) x
#else
# define __P(x) ()
#endif
