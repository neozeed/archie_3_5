#include <stdio.h>
#ifdef ARCHIE_TIMING
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <psrv.h>
#include <plog.h>
#include <pprot.h>
#include <perrno.h>
#include <pmachine.h>
#include "protos.h"


#define	 NUM_BUCKETS   13
#define  NUM_REQUESTS  10

typedef struct{
   int time_taken[NUM_REQUESTS];
   int curridx;
} est_t;

/* Set up 13 "buckets" to hold the time estimates */

/* Bucket #   Priority #
     0		 0
     1		 1
     2		 2
     3		 3
     4		 4
     5		 5 - 100
     6		 101 - 699
     7		 700 - 799
     8		 800 - 899
     9		 900 - 999
     10		 1000 - 2000
     11		 2001 - 9999
     12		 >10000 
*/

est_t  est_array[NUM_BUCKETS];


#ifdef ARCHIE_TIMING
int diff_time(a,b,c) /* does c = a-b */
struct timeval *a,*b,*c;
{
   c->tv_usec = a->tv_usec - b->tv_usec;
   c->tv_sec = a->tv_sec - b->tv_sec;
   if ( c->tv_sec < 0 ) {
      c->tv_sec = c->tv_usec = 0;
   }
   if ( c->tv_usec < 0 )  {
      if ( c->tv_sec > 0 ) 
	 c->tv_sec--;
      c->tv_usec += 1000000;
   }
}
#endif

void arch_set_etime(nreq)
    RREQ nreq;
{
   int p = nreq -> pf_priority;
   int t = time((time_t *) NULL) - nreq -> rcvd_time.tv_sec;
   int idx;

#ifdef ARCHIE_TIMING
   struct timeval lap;
   static FILE *timelog;
   struct tm *tmptr;
   time_t tt;

   time(&tt);
   tmptr = localtime(&tt);
   strcpy(nreq->end,asctime(tmptr));
#endif

   if(p < 5){
      idx = p;
   } else if(p < 101){
      idx = 5;
   } else if(p < 700){
      idx = 6;
   } else if(p < 800){
      idx = 7;
   } else if(p < 900){
      idx = 8;
   } else if(p < 1000){
      idx = 9;
   } else if(p < 2000){
      idx = 10;
   } else if(p < 10000){
      idx = 11;
   } else {
      idx = 12;
   }

   est_array[idx].time_taken[est_array[idx].curridx++] = t;
   est_array[idx].curridx %= NUM_REQUESTS;

#ifdef ARCHIE_TIMING
   if ( nreq->match_type != 'X' ) {
      if ( timelog == NULL ) {
	 timelog = fopen("/pfs/time.log","a");
	 if ( timelog == NULL ) {
	    return;
	 }
      }

      if ( nreq->query_state != ' ') {

      fprintf(timelog,"start  %s",nreq->start);
      fprintf(timelog,"launch %s",nreq->launch);
      fprintf(timelog,"end    %s",nreq->end);

      fprintf(timelog,"status %c %c %c %c %c %c\n",nreq->query_state,
              nreq->cached,nreq->match_type, nreq->case_type, 
              nreq->domain_match,nreq->path_match);

      diff_time(&(nreq->qtime_end.ru_utime),&nreq->qtime_start.ru_utime,&lap);
      fprintf(timelog,"qtime %ld.%02ld ", lap.tv_sec,lap.tv_usec/10000);
      diff_time(&nreq->qtime_end.ru_stime,&nreq->qtime_start.ru_stime,&lap);
      fprintf(timelog,"%ld.%02ld\n", lap.tv_sec,lap.tv_usec/10000);

      diff_time(&nreq->stime_end.ru_utime,&nreq->stime_start.ru_utime,&lap);
      fprintf(timelog,"stime %ld.%02ld ", lap.tv_sec,lap.tv_usec/10000);
      diff_time(&nreq->stime_end.ru_stime,&nreq->stime_start.ru_stime,&lap);
      fprintf(timelog,"%ld.%02ld\n", lap.tv_sec,lap.tv_usec/10000);

      diff_time(&nreq->htime_end.ru_utime,&nreq->htime_start.ru_utime,&lap);
      fprintf(timelog,"htime %ld.%02ld ", lap.tv_sec,lap.tv_usec/10000);
      diff_time(&nreq->htime_end.ru_stime,&nreq->htime_start.ru_stime,&lap);
      fprintf(timelog,"%ld.%02ld ", lap.tv_sec,lap.tv_usec/10000);
      fprintf(timelog,"%d %d\n",nreq->hosts,nreq->hosts_searched);

      fprintf(timelog,"query %d \"%s\"\n",nreq->no_matches,nreq->search_str);
      fprintf(timelog,"\n");
      fflush(timelog);
      }
   }
#endif

}

time_t arch_get_etime(nreq)
    RREQ nreq;
{
   int p = nreq -> pf_priority;
   int idx;
   int total;
   int i;
   int retval;

#ifdef ARCHIE_TIMING
   struct tm *tmptr;
   time_t tt;

   nreq->query_state = ' ';
   time(&tt);
   tmptr = localtime(&tt);
   strcpy(nreq->start,asctime(tmptr));
#endif

   
   if(p < 5){
      idx = p;
   } else if(p < 101){
      idx = 5;
   } else if(p < 700){
      idx = 6;
   } else if(p < 800){
      idx = 7;
   } else if(p < 900){
      idx = 8;
   } else if(p < 1000){
      idx = 9;
   } else if(p < 2000){
      idx = 10;
   } else if(p < 10000){
      idx = 11;
   } else {
      idx = 12;
   }

   for(i =0, total = 0; i < NUM_REQUESTS; i++)
      total += est_array[idx].time_taken[i];

   if((retval = total / 10) < 5)
      retval = 5;   /* Always return a minimum of 5 seconds */

   plog(L_DB_INFO,nreq,"Est time: %d", retval);

   return(retval);

}

#if 0

char *print_queue_timings()

{
   static char retstr[256];

   int i, j;
   char tstr[256];
   int total;
   char *priority_str[] = {{"        0"},{"        1"},{"       2"},{"       3"},{"       4"},{"      5-100"},{"  101-699"},
			  {" 700-799"},{" 801-899"},{" 901-999"},{"1000-1999"},{"2000-9999"},{"  >10000"}};

   retstr[0] = '\0';

   for(j = 0; j < NUM_BUCKETS; j++){

      for(i =0, total = 0; i < NUM_REQUESTS; i++)
	 total += est_array[j].time_taken[i];

      sprintf(tstr, "%s:%03d ", priority_str[j], total / NUM_REQUESTS);

      if(j == 5)
	 strcat(retstr, "\n       ");

      if(j ==10)
	 strcat(retstr, "\n         ");

      strcat(retstr, tstr);
   }

   return(retstr);
}

#endif


char *print_queue_timings()

{
   static char retstr[NUM_BUCKETS*100]; 

   int i, j;
   char tstr[NUM_BUCKETS*100]; 
   int total;
   char *priority_str[] = {"0", "1", "2", "3", "4", "5-100", "101-699",
                           "700-799", "801-899", "901-999", "1000-1999",
                           "2000-9999", ">10000"};

   retstr[0] = '\0';

   for(j = 0; j < NUM_BUCKETS; j++){

      for(i =0, total = 0; i < NUM_REQUESTS; i++)
	 total += est_array[j].time_taken[i];

      sprintf(tstr, "Timings: %-10s%15d\n", priority_str[j], total / NUM_REQUESTS);
      strcat(retstr, tstr);
   }

   return(retstr);
}
