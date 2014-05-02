#include "os_interface.h"

#include <time.h>


int get_tick_count()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
	{
 		// error
	}
	return 1000 * ts.tv_sec + ts.tv_nsec / 1000000;
}
