/*
 * Copyright (c) 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the file
 * <usc-copyr.h>.
 *
 * The menu API and client were written by Kwynn Buess (buess@isi.edu)
 */

#include <usc-copyr.h>



#include <pfs.h>

int compare_collation_ord(VLINK v1,VLINK v2) { 
  struct pattrib *get_high_coll_ord(PATTRIB);
  PATTRIB coll1 = get_high_coll_ord(v1->lattrib);
  PATTRIB coll2 = get_high_coll_ord(v2->lattrib);
  int temp;
  int compare_name(VLINK,VLINK);
  struct token *seq_1;
  struct token *seq_2;

  /* Excluding ASCII and "LAST" (not implemented yet), there are three 
     possible states for each link -- NO-COLL-ATTRIB, LAST, or what I'll 
     call "first" (i.e. not LAST).  There are three states for each of the
     two links, so there are six possible combinations.  Here is a table of 
     them.  I have represented NO-COLLATION-ATTRIBUTE with "___", 
     LAST with 'L', and "first" with 'F'.


     Possible combinations of COLLATION-ATTRIBUTES for two links

     1:  ___ ___
     2:  L   ___
     3:  F   ___
     4:  L   F
     5:  L   L
     6:  F   F
*/


  if ((coll1 == NULL) && (coll2 == NULL)) return compare_name(v1,v2);
  /* there goes case #1 */

  if (coll1 == NULL) return 
    (!strcmp(coll2->value.sequence->token,"LAST")) ? 1 : -1;
  if (coll2 == NULL) return 
      (!strcmp(coll1->value.sequence->token,"LAST")) ? -1 : 1;
  /* there go #2 and #3 */

  
  /* Now we no that neither of the collation pointers are NULL. */

  seq_1 = coll1->value.sequence;
  seq_2 = coll2->value.sequence;

/* Now there's the degenerate case of a non-NULL COLL-ATTRIB with a 
   NULL value.sequence.  The spec says that this should be treated just 
   like a NULL COLL-ATTRIB.

   Now we repeat the above performance, replacing "coll"'s with "seq"'s
 */

  if ((seq_1 == NULL) && (seq_2 == NULL)) return compare_name(v1,v2);
  if (seq_1 == NULL) return 
    (!strcmp(seq_2->token,"LAST")) ? 1 : -1;
  if (seq_2 == NULL) return 
    (!strcmp(seq_1->token,"LAST")) ? -1 : 1;

/* This code determines if EITHER the COLL-ATTRIBS are BOTH "FIRST" 
   or both "LAST".  If not, then one of them is last (case #4), and
   we return that as lower.  Note that those are the only three 
   cases left.  
*/
  if (
      (temp = (!(strcmp(seq_1->token,"LAST"))))
      != (!(strcmp(seq_2->token,"LAST")))
      ) return temp ? -1 : 1;


/* Now we're down to BOTH LAST's and both FIRST's.  The process for 
   comparing these is the same.  However, with LAST we must move 
   beyond the word "LAST" to get to "NUMERIC" or "ASCII".  
   So we advance the pointer.
*/
  if (!strcmp(seq_1->token,"LAST")) {
    seq_1 = seq_1 -> next;
    seq_2 = seq_2 -> next;

/* This is the case of LAST with no arguments.  If both are of type "LAST", 
   then they remain in the former order (1 comes before 2).  
*/
    if (seq_1 == NULL && seq_2 == NULL) return 1;
    
/* If one is "LAST" and the other is "LAST ASCII" or "LAST NUMERIC", then 
   the one that is "LAST" goes last;
*/
    if ((temp = (seq_1 == NULL)) || (seq_2 == NULL)) 
      temp ? -1 : 1;

  }
  
/* Now we are down to NUMERIC or ASCII */


/* If one's ASCII and one's NUMERIC, the NUMERIC comes first */
  if ((temp = (!strcmp(seq_1->token,"NUMERIC")))
      != (!strcmp(seq_1->token,"NUMERIC"))
       ) return temp ? 1 : -1;
/* After that we know that we have both NUMERIC or both ASCII */


/* We know that they're both NUMERIC or BOTH ASCII.  Whether they're LAST 
   NUMERIC or LAST ASCII or "FIRST" NUMERIC and "FIRST" ASCII makes no 
   difference.  
*/


  if (!strcmp(seq_1->token,"ASCII")) { 

    /* Move past the word ASCII */
    seq_1 = seq_1 -> next;
    seq_2 = seq_2 -> next;


    while((seq_1 != NULL) && (seq_2 != NULL)) { 
      if (temp = strcmp(seq_1->token,seq_2->token)) break;
      seq_1 = seq_1 -> next;
      seq_2 = seq_2 -> next;
    }
    
    if ((seq_1 == NULL) && (seq_2 == NULL)) 
      return compare_name(v1,v2);

    /* If one was NULL, the one that ends first is first */
    if ((seq_1 == NULL) || (seq_2 == NULL)) 
      return (seq_1 == NULL) ? 1: -1;

    /* seq_1 and seq_2 were both non-NULL and non-equal.  Determine
       the inequality and return.         */

    return ((temp < 0) ? 1:-1);

   

  }

/* We now assume that only NUMERIC is left.  If this isn't the case, 
   something's very wrong.  Perhaps the attribute is malformed */


/* Move past the word NUMERIC */
    seq_1 = seq_1 -> next;
    seq_2 = seq_2 -> next;

/* Compare numbers until there are no more or they are unequal */
    while ((seq_1 != NULL) && (seq_2 != NULL)) { 
      if (!((atoi(seq_1->token) == atoi(seq_2->token)))) break;
      seq_1 = seq_1 -> next;
      seq_2 = seq_2 -> next;
    }
    
  if ((seq_1 == NULL) && (seq_2 == NULL)) 
    return compare_name(v1,v2);

/* If one was NULL, the one that ends first is first */
    if ((seq_1 == NULL) || (seq_2 == NULL)) 
      return (seq_1 == NULL) ? 1: -1;

/* If the loop above ended due to an inequality, determine it.  */
    return (atoi(seq_1->token) > atoi(seq_2->token)) ? -1: 1;

}



PATTRIB get_high_coll_ord(PATTRIB head) { 
  int compare_precedence(char,char);
  PATTRIB temp = head;
  PATTRIB highest = NULL;


  while (temp != NULL) { 
    if (!strcmp(temp->aname,"COLLATION-ORDER")) {
      if (highest == NULL) highest = temp;
      else if 
	(compare_precedence(highest->precedence,temp->precedence) == -1) 
	  highest = temp;
    }
    temp = temp->next;
  }

  return highest;
}

int compare_name(VLINK a,VLINK b) { 
  int cnt;

  for (cnt=0; 
       ((a->name[cnt]) == (b->name[cnt])) 
       && (a->name[cnt] != '\0') && (b->name[cnt] != '\0'); 
       cnt++) /* NULL loop body */ 
    ;
  
  return (b->name[cnt] < a->name[cnt]) ? -1:1; 
}


int compare_precedence(char p1,char p2) {


  
  if (p1 == ATR_PREC_LINK   ) return  1;
  if (p2 == ATR_PREC_LINK   ) return -1;

  if (p1 == ATR_PREC_REPLACE) return  1;
  if (p2 == ATR_PREC_REPLACE) return -1;

  if (p1 == ATR_PREC_OBJECT ) return  1;
  if (p2 == ATR_PREC_OBJECT ) return -1;

  if (p1 == ATR_PREC_CACHED ) return  1;
  if (p2 == ATR_PREC_CACHED ) return -1;

  if (p1 == ATR_PREC_ADD    ) return  1;
  if (p2 == ATR_PREC_ADD    ) return -1;

  return 1;
}
