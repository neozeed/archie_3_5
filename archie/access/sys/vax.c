#include	<sys/types.h>
#include	<sys/times.h>

int hz = 60;
double tcmp = 3.86;

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
