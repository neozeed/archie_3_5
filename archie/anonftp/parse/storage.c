/*
  We assume that a series of memory allocations of fixed length are to be
  done, followed by a single 'free' of all allocated memory.

  We provide routines to allocate an element.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "parse.h"
#include "error.h"

/* default number of elements in a block */
#define NELTS 512

typedef struct block Block ;
struct block
{
  void *mem ;                   /* pointer to first entry in block */
  void *end ;                   /* pointer to last entry in block */

  struct block *next ;          /* pointer to next block */
} ;

typedef struct header Header ;
struct header
{
  int elts_per_blk ;            /* counter of number of elements in each block */
  int elt_size ;                /* size of each element */

  int block_size ;              /* derived: size of each block of elements */

  int curr_elt ;
  int total_elts ;              /* number of elements used in all blocks */

  void *next_elt ;              /* pointer to next available element */

  Block *first ;                /* pointer to first block of elements */
  Block *last ;                 /* pointer to last (current) block of elements */
} ;

static Header hd ;


int
free_elts()
{
  Block *b ;

  mck ;
  for(b = hd.first ; b != (Block *)0 ; )
  {
    Block *tmp;
    
    free(b->mem) ;
    tmp = b->next;
    free((void *)b) ;
    b = tmp;
  }
  hd.total_elts = hd.elts_per_blk = hd.elt_size = hd.block_size = 0 ;
  hd.curr_elt = -1;
  hd.next_elt = (void *)0 ;
  hd.first = hd.last = (Block *)0 ;
  mck ;
  return 1 ;
}


int elt_num()
{
  return hd.curr_elt ;
}


static Block *
new_block()
{
  Block *b ;

  mck ;
  if((b = (Block *)malloc(sizeof(Block))) != (Block *)0)
  {
    void *p ;

    if((p = malloc((size_t)hd.block_size)) == (void *)0)
    {
      free((void *)b) ;
      b = (Block *)0 ;
    }
    else
    {
      b->next = (Block *)0 ;
      b->mem = (Block *)p ;
      b->end = (void *)((char *)b->mem + (hd.elts_per_blk - 1) * hd.elt_size) ;
    }
  }
  mck ;
  return b ;
}


/*
    Upon entry hd.next_elt points to the next unused memory location.
*/

void *
new_elt()
{
  void *p = hd.next_elt ;

  mck ;
  if(hd.next_elt < hd.last->end)
  {
    hd.next_elt = (void *)((char *)hd.next_elt + hd.elt_size) ;
  }
  else
  {
    Block *b = new_block() ;

    if(b == (Block *)0)
    {
      return (void *)0 ;
    }
    else
    {
      hd.next_elt = b->mem ;

      hd.last->next = b ;
      hd.last = b ;
    }
  }
  hd.curr_elt++ ;
  hd.total_elts++ ;
  mck ;
  memset(p,0,hd.elt_size);
  return p ;
}


static Block *curr_blk = (Block *)0 ;
static void *curr_mem = (void *)0 ;

void *
first_elt()
{
  void *p = (void *)0 ;

  mck ;
  if(hd.first == (Block *)0)
  {
    error(A_ERR, "first_elt", "storage has not been initialized") ;
  }
  else if(hd.total_elts > 0)
  {
    curr_blk = hd.first ;
    p = curr_mem = hd.first->mem ;
    hd.curr_elt = 0 ;
  }
  mck ;
  return p ;
}


/*
    Upon entry curr_elt is the number of elements returned and curr_mem points to the last
    entry returned.
*/

void *
next_elt()
{
  void *p = (void *)0 ;

  mck ;
  if(hd.curr_elt < hd.total_elts - 1) /* hd.curr_elt counts from 0 */
  {
    if(curr_mem != curr_blk->end)
    {
      hd.curr_elt++ ;
      p = curr_mem = (void *)((char *)curr_mem + hd.elt_size) ;
    }
    else                        /* go to next block */
    {
      if((curr_blk = curr_blk->next) != (Block *)0) /* have we done the last block? */
      {
        hd.curr_elt++ ;
        p = curr_mem = curr_blk->mem ;
      }
    }
  }
  mck ;
  return p ;
}


int
set_elt_size(size)
  size_t size ;
{
  mck ;
  if(hd.elt_size != 0)
  {
    error(A_ERR, "set_elt_type", "element type has already been set") ;
    return 0 ;
  }
  else
  {
    Block *b ;

    hd.elt_size = size ;
    hd.elts_per_blk = NELTS ;

    hd.block_size = hd.elt_size * hd.elts_per_blk ;

    if((b = new_block()) != (Block *)0)
    {
      hd.total_elts = 0 ;
      hd.next_elt = b->mem ;
      hd.first = hd.last = b ;
    }

    hd.curr_elt = -1 ;          /* cuz we want to start at 0 */

    mck ;
    return b != (Block *)0 ;
  }
}
