#include <ghost.h>
#include <errno.h>
#include "coconut.h"
#include "stdio.h"

__thread int bar = 54;
__thread int bee = 31;

void coconutThrow()
{
	klog("library: errno from libc.so");
	klog("errno %x = %i", &errno, errno);
	errno = 321;
	klog("-> %i == 321", errno);

	klog("library: bar from local");
	klog("bar %x = %i", &bar, bar);
	bar = 25;
	klog("-> %i == 25", bar);

	klog("library: bee from local");
	klog("bee %x = %i", &bar, bar);
	bar = 13;
	klog("-> %i == 13", bee);

	klog("throwing exception...");
	throw 20;
}
