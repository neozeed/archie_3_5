#ifndef QUEUE_DEF_H
#define QUEUE_DEF_H

#include "info_cell.h"

typedef struct Queue_elt Queue_elt ;

struct Queue_elt
{
  Info_cell *ident ;

  Queue_elt *next ;
  Queue_elt *prev ;
} ;

typedef struct queue_t
{
  Queue_elt *head ;
  Queue_elt *tail ;
} Queue ;

#endif
