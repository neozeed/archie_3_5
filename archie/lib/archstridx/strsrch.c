#include <ctype.h>
#include <string.h>
#include "strsrch.h"

#define NCHARS 256          /* alphabet size */

#if 0
/*  
 *  Boyer-Moore-Horspool string search.  Search for key[1..klen] in
 *  text[1..tlen].
 *  
 *  From page 228 of "Information Retrieval: Data Structures & Algorithms",
 *  edited by William B. Frakes and Ricardo Baeza-Yates.
 *  
 *  This is the original version, supplied as a reference.
 */
int bmhSearch(text, n, pat, m)
  char text[], pat[];
  int n, m;
{
  int d[MAX_ALPHABET_SIZE], i, j, k, lim;

  for (k = 0; k < MAX_ALPHABET_SIZE; k++) d[k] = m + 1;
  for (k = 1; k <= m; k++) d[pat[k]] = m + 1 - k;
  pat[m+1] = CHARACTER_NOT_IN_THE_TEXT; /* To avoid having code
                                           for special case n-k+1=m */

  lim = n - m + 1;
  for (k = 1; k <= lim; k += d[text[k + m]]) /* Searching */
  {
    i = k;
    for (j = 1; text[i] == pat[j]; j++) i++;
    if (j == m + 1) Report_match_at_position(k);
  }
  /* restore pat[m+1] if necessary */
}
#endif


/*  
 *  Like _archBMHSearch() (see below), but ignores case.
 *  
 *  bug: could possibly speed this up by creating two copies of the key, one
 *  entirely upper case, the other entirely lower case, then using them both
 *  in the tight loop.
 */
void _archBMHCaseSearch(const char *text, unsigned long tlen, const char *key, unsigned long klen,
                        unsigned long maxhits, unsigned long *nhits, unsigned long start[])
{
  long d[NCHARS], i, j, k, lim;

  *nhits = 0;
  if (maxhits <= 0) {
    return;
  }

  for (k = 0; k < NCHARS; k++) {
    d[k] = klen + 1;
  }
  for (k = 0; k < klen; k++) {
    d[(unsigned char)tolower(key[k])] = klen - k;
    d[(unsigned char)toupper(key[k])] = klen - k;
  }

  lim = tlen - klen;

  /*  
   *  This value of `lim' ensures we don't use the optimized loop to compare
   *  the key with the text when the end of the key is aligned with the end
   *  of the text.  A match in that case would cause us to run off the ends
   *  of both arrays.
   */

  for (k = 0; k < lim && *nhits < maxhits; k += d[(unsigned char)text[k + klen]]) {
    i = k;
    for (j = 0; tolower(text[i]) == tolower(key[j]); j++) {
      i++;
    }
    if (j == klen) {
      start[(*nhits)++] = k;
      if (*nhits == maxhits) {
        return;
      }
    }
  }

  /*  
   *  Check for the special case of the end of the key being aligned with the
   *  end of the text.
   */
  
  if (k == tlen - klen && strncasecmp(text + k, key, klen) == 0) {
    start[(*nhits)++] = k;
    if (*nhits == maxhits) {
      return;
    }
  }
}


/*  
 *  Boyer-Moore-Horspool string search.  Search for key[0..klen-1] in
 *  text[0..tlen-1].
 *  
 *  Assume `key' is nul-terminated and that nul does not appear as part of
 *  text (except, perhaps, as a terminator).
 *  
 *  Based on the code from page 228 of "Information Retrieval: Data
 *  Structures & Algorithms", edited by William B. Frakes and Ricardo
 *  Baeza-Yates.
 */  
void _archBMHSearch(const char *text, unsigned long tlen, const char *key, unsigned long klen,
                    unsigned long maxhits, unsigned long *nhits, unsigned long start[])
{
  long d[NCHARS], i, j, k, lim;

  *nhits = 0;
  if (maxhits <= 0) {
    return;
  }

  for (k = 0; k < NCHARS; k++) {
    d[k] = klen + 1;
  }
  for (k = 0; k < klen; k++) {
    d[(unsigned char)key[k]] = klen - k;
  }

  lim = tlen - klen;

  /*  
   *  This value of `lim' ensures we don't use the optimized loop to compare
   *  the key with the text when the end of the key is aligned with the end
   *  of the text.  A match in that case would cause us to run off the ends
   *  of both arrays.
   */

  for (k = 0; k < lim && *nhits < maxhits; k += d[(unsigned char)text[k + klen]]) {
    i = k;
    for (j = 0; text[i] == key[j]; j++) {
      i++;
    }
    if (j == klen) {
      start[(*nhits)++] = k;
      if (*nhits == maxhits) {
        return;
      }
    }
  }

  /*  
   *  Check for the special case of the end of the key being aligned with the
   *  end of the text.
   */
  
  if (k == tlen - klen && strncmp(text + k, key, klen) == 0) {
    start[(*nhits)++] = k;
    if (*nhits == maxhits) {
      return;
    }
  }
}



/*  
 *  Like _archBMHSearch() (see below), but ignores case.
 *  
 *  bug: could possibly speed this up by creating two copies of the key, one
 *  entirely upper case, the other entirely lower case, then using them both
 *  in the tight loop.
 */
void _archBMHGeneralSearch(const char *text, unsigned long tlen, const char *key, unsigned long klen,
                           unsigned long maxhits, unsigned long *nhits, unsigned long start[],
                           struct funcs funs)
{
  long d[NCHARS], i, j, k, lim;

  *nhits = 0;
  if (maxhits <= 0) {
    return;
  }

  for (k = 0; k < NCHARS; k++) {
    d[k] = klen + 1;
  }
  for (k = 0; k < klen; k++) {
    d[(unsigned char)funs.lowerCase(key[k])] = klen - k;
    d[(unsigned char)funs.upperCase(key[k])] = klen - k;
    d[(unsigned char)funs.withAccent(key[k])] = klen - k;
    d[(unsigned char)funs.noAccent(key[k])] = klen - k;
  }

  lim = tlen - klen;

  /*  
   *  This value of `lim' ensures we don't use the optimized loop to compare
   *  the key with the text when the end of the key is aligned with the end
   *  of the text.  A match in that case would cause us to run off the ends
   *  of both arrays.
   */

  for (k = 0; k < lim && *nhits < maxhits; k += d[(unsigned char)text[k + klen]]) {
    i = k;
    for (j = 0;
         funs.lowerCase(funs.noAccent(text[i])) ==
         funs.lowerCase(funs.noAccent(key[j])); j++) {
      i++;
    }
    if (j == klen) {
      start[(*nhits)++] = k;
      if (*nhits == maxhits) {
        return;
      }
    }
  }

  /*  
   *  Check for the special case of the end of the key being aligned with the
   *  end of the text.
   */
  
  if (k == tlen - klen && funs.toComp(text + k, key) == 0) {
    start[(*nhits)++] = k;
    if (*nhits == maxhits) {
      return;
    }
  }
}

