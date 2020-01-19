#include <ghost.h>
#include <errno.h>
#include "coconut.h"
#include "stdio.h"

__thread int bar = 54;

void coconutThrow()
{
	klog("errno %x = %i", &errno, errno);
	errno = 321;
	klog("%i", errno);

	klog("bar %x = %i", &bar, bar);
	bar = 25;
	klog("%i == 25", bar);

	klog("throwing exception...");
	throw 20;
}
