#include "defs.h"
#include "timing.h"


void _patrieTimingStart(struct _patrie_timing *t)
{
  times(&t->start);
}


void _patrieTimingEnd(struct _patrie_timing *t)
{
  times(&t->end);
}


/*
 *  Return the amount of time, in seconds, recorded by `*t'.
 */
double _patrieTimingDiff(struct _patrie_timing *t)
{
  clock_t stime = t->end.tms_stime - t->start.tms_stime;
  clock_t utime = t->end.tms_utime - t->start.tms_utime;
  static long clk_tck = 0;

  if (clk_tck == 0 && (clk_tck = sysconf(_SC_CLK_TCK)) == -1) {
    fprintf(stderr, "%s: _patrieTimingDiff: can't get clock ticks: ", prog);
    perror("sysconf");
    return 0.0;
  }

  return (stime + utime) / (double)clk_tck;
}
