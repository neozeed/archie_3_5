#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>

int hz = HZ;
double tcmp = 4.91;

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
