#include <signal.h>
#include <stdio.h>
#include "queue.h"

#ifdef __STDC__
extern void sig(int s) ;
#else
extern void sig(int s) ;
#endif


char *prog = "tq" ;
volatile int trap = 0 ;

int main()
{
  char s[128] ;
  int i ;
  Queue_elt *qe ;
  Queue *q1 = new_Queue() ;
  Queue *q2 = new_Queue() ;

  signal(SIGINT, sig) ;
  while(trap == 0)
  {
    /*	fprintf(stderr, "start\n") ; sleep(1) ;*/
    for(i = 0 ; i < 1 ; i++)
    {
      if((qe = new_Queue_elt(sprintf(s, "foo %d", i))) == (Queue_elt *)0)
      {
        fprintf(stderr, "new_queue_elt failed.\n") ;
        exit(1) ;
      }
      if( ! addq(qe, q1))
      {
        fprintf(stderr, "addq failed.\n") ;
        exit(1) ;
      }
    }
    /*	fprintf(stderr, "created 1\n") ;*/
    while((qe = remq(q1)) != (Queue_elt *)0)
    {
      addq(qe, q2) ;
    }
    /*	fprintf(stderr, "moved 1 to 2\n") ;*/
    while((qe = remq(q2)) != (Queue_elt *)0)
    {
      dispose_Queue_elt(qe) ;
    }
    /*	fprintf(stderr, "disposed of 2\n") ;*/
  }
  printq("q1", q1) ;
  printq("q2", q2) ;
  return 1 ;
}


int
printq(s, q)
  char *s ;
  Queue *q ;
{
  Queue_elt *qe = q->head ;

  ptr_check(s, char, "printq", 0) ;

  while(qe != (Queue_elt *)0)
  {
    printf("%s: %s\n", s, qe->str) ;
    qe = qe->prev ;
  }
  return 1 ;
}


void sig(s)
  int s ;
{
  trap = 1 ;
}
