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

#ifndef list_h
#define list_h

#include "cutil.h" 

/*---------------------------------------------------------------------------*/

typedef struct listStruct
{ void** elems;
  long len;     
  long rlen;    
} listStruct;

#define list listStruct*

#define length(l)         (long)(((l) == NULL) ? 0L : (l)->len)
#define nth(l,n)  \
  (((l) == NULL || length((l)) <= (n)) ? NULL : (l)->elems[(n)]) 
#define car(l)            (nth((l),0L))
#define cadr(l)           (nth((l),2L))
#define cdr(l)            (removeNth((l),0L)) 
#define first(l)          (car(l))
#define last(l)           (nth((l),length((l))-1L))
#define rest(l)           (cdr(l))
#define setfNth(l,n,elem) \
  (((l) == NULL || length((l)) <= (n)) ? 0 : ((l)->elems[(n)] = (elem)))
#define setfCar(l,elem)   (setfNth((l),0L,(elem)))
#define null(l)           (((l) == NULL || length((l)) == 0L) ? true : false)

void mapcar(list l,void function (void* argument));
list collecting(list l,void *item); 
void freeList(list l);
void sortList(list l,long (*cmp)(void* arg1,void* arg2));
list insertNth(list l,long n,void* elem);
list removeNth(list l,long n);
boolean lookupInSortedList(list l,void* val,
			long (*cmpFunc)(void* arg1,void* arg2),
			long* posInList);
void* pop(list* l);
void  push(list* l,void* elem);

/*---------------------------------------------------------------------------*/

#endif 

