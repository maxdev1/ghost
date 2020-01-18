#include <ghost.h>
#include "coconut.h"
#include "stdio.h"

__thread int y;

void coconutThrow()
{
	klog("Lib writing a thread-local value...");
	y = 3;
	klog("Lib wrote thread-local value: %i", y);

	klog("Lib throws exception");
	throw 20;
}
