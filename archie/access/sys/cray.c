#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/machd.h>

int hz = HZ;
double tcmp = 3.34;

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
