#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>

#if !defined(AIX) && !defined(SOLARIS)
int hz = HZ;
#endif
double tcmp = 3.04;

#if 0
static struct tms start;

startclock()
{
	times(&start);
}

stopclock()
{
	struct tms t;

	times(&t);
	return(t.tms_utime - start.tms_utime);
}
#endif
