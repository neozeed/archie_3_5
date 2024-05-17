#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>


struct _patrie_timing {
  struct tms start;
  struct tms end;
};


extern double _patrieTimingDiff(struct _patrie_timing *t);
extern void _patrieTimingEnd(struct _patrie_timing *t);
extern void _patrieTimingStart(struct _patrie_timing *t);
