#include <ghost.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include "coconut.h"

__thread int bar = 54;
__thread int bee = 31;

class LibGlobCtorTest {
public:
	int x = 5;
	LibGlobCtorTest() {
		assert(x == 5);
		x = 12;
	}
};

LibGlobCtorTest libctortest;

int libcctor = 12;
__attribute__ ((constructor)) void libGlobCCtor(void)
{
	assert(libcctor == 12);
	libcctor = 43;
}

void coconutThrow(int x)
{
	assert(libcctor == 43);

	assert(errno == 123);
	errno = 321;
	assert(errno == 321);

	assert(bar = 54);
	bar = 25;
	assert(bar = 25);

	assert(bee == 31);
	bee = 13;
	assert(bee == 13);

	assert(libctortest.x == 12);

	g_log("throw");
	throw x * 2;
}
