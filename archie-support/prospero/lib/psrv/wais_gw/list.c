/*****************************************************************************
* 	        (c) Copyright 1992 Wide Area Information Servers, Inc        *
*	   	  of California.   All rights reserved.   	             *
*									     *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.		     *
*									     *
*  Wide Area Information Server software is the proprietary and              *
*  confidential property of Wide Area Information Servers, Inc.              *
*****************************************************************************/

#include "list.h"

/*---------------------------------------------------------------------------*/

void 
mapcar(list l,void (*function)())
{
  long i;

  if (null(l))
    return;

  for (i = 0; i < length(l); i++)
   { (*function)(nth(l,i));
   }
}

/*---------------------------------------------------------------------------*/

list
collecting(list l,void *item)

{
  if (l == NULL)
    if (!(l = (list)s_malloc(sizeof(listStruct))))
	return NULL;

  if (l->rlen < length(l) + 1)
   { l->rlen = length(l) + 2;
     l->elems = (void**)s_realloc(l->elems,l->rlen * sizeof(void*));
   }

  l->elems[length(l)] = item;
  l->len++;

  return(l);
}

/*---------------------------------------------------------------------------*/

void
freeList(list l)
{
  if (l != NULL)
   { s_free(l->elems);
     s_free(l);
   }
}

/*---------------------------------------------------------------------------*/

static long 
cmpStrings(void* arg1,void* arg2)
{
  return(strcmp(*(char**)arg1,*(char**)arg2));
}

/*---------------------------------------------------------------------------*/

void
sortList(list l,long (*cmp)())
{ 
  if (cmp == NULL)
    cmp = cmpStrings;

  if (length(l) > 1)
    qsort(l->elems,length(l),sizeof(void*),cmp);
}

/*---------------------------------------------------------------------------*/

list 
insertNth(list l,long n,void* elem)
{
  if (null(l))
    return(NULL);

  if (n > length(l)) 
    return(NULL);

  if (l->rlen < length(l) + 1)
   { l->rlen = length(l) + 2;
     l->elems = (void**)s_realloc(l->elems,l->rlen*sizeof(void*));
   }

  memmove(l->elems+n+1,l->elems+n,(length(l)-n)*sizeof(void*));
  l->elems[n] = elem;
  l->len++;

  return(l);
}

/*---------------------------------------------------------------------------*/

list
removeNth(list l,long n)
{
  if (null(l))
    return(NULL);

  if (n >= length(l)) 
    return(NULL);

  memmove(l->elems+n,l->elems+n+1,(length(l)-n-1)*sizeof(void*));
  l->len--;
  
  if (l->rlen > 4 && l->rlen > length(l) * 2)
   { l->rlen = length(l);
     l->elems = (void**)s_realloc(l->elems,l->rlen * sizeof(void*));
   }

  return(l);
}

/*---------------------------------------------------------------------------*/

boolean
lookupInSortedList(list l,void* val,long (*cmpFunc)(void* arg1,void* arg2),
		   long* posInList)

{
  long pos;
  long upperBound = length(l);
  long lowerBound = 0;
  long cmp;
  void* data;

  if (null(l))
   { if (posInList != NULL)
       *posInList = 0;
     return(false);
   }

  if (cmpFunc == NULL)
    cmpFunc = cmpStrings;

  while (upperBound >= lowerBound)
   { 
     pos = lowerBound + ((upperBound - lowerBound) / 2);

     if (pos == length(l))
       cmp = -1;
     else
      { data = nth(l,pos);
	cmp = (*cmpFunc)(&val,&data);
      }

     if (cmp == 0)
      { if (posInList != NULL)
	  *posInList = pos;
	return(true);
      }
     else if (cmp < 0) 
      { if (pos == lowerBound) 
	 { if (posInList != NULL)
	     *posInList = pos;
	   return(false);
	 }
	else
	  upperBound = pos; 
      }
     else 
      { if (pos >= length(l))
	 { if (posInList != NULL)
	     *posInList = pos;
	   return(false);
	 }
	else
	  lowerBound = pos + 1; 
      }
   }

  if (posInList != NULL)
    *posInList = length(l); 
  return(false);
}

/*---------------------------------------------------------------------------*/

void*
pop(list* l)
{ 
  void* ans;

  if (l == NULL || *l == NULL)
    return(NULL);

  ans = first(*l);
  *l = removeNth(*l,0);
  return(ans);
}

/*---------------------------------------------------------------------------*/

void 
push(list* l,void* elem)
{
  if (l == NULL)
    return;

  *l = insertNth(*l,0,elem);

}

/*---------------------------------------------------------------------------*/
