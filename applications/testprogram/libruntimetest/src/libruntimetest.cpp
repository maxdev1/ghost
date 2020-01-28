#include <ghost.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include "libruntimetest.h"

/**
 * Tests thread-local variables.
 */
__thread int threadLocalTest1 = 54;
__thread int threadLocalTest2 = 31;

void testLibraryThreadLocal()
{
	assert(threadLocalTest1 = 54);
	threadLocalTest1 = 25;
	assert(threadLocalTest1 = 25);

	assert(threadLocalTest2 == 31);
	threadLocalTest2 = 13;
	assert(threadLocalTest2 == 13);
}

/**
 * Tests C++-style global constructors.
 */
class LibraryGlobalConstructorTestClass {
public:
	int field = 5;
	LibraryGlobalConstructorTestClass() {
		assert(field == 5);
		field = 12;
	}
};

LibraryGlobalConstructorTestClass libraryGlobalCppConstructorTest;

void testLibraryGlobalCppConstructors()
{
	assert(libraryGlobalCppConstructorTest.field == 12);
}

/**
 * Tests C-style global constructors.
 */
int libraryGlobalConstructorVariable = 12;
__attribute__ ((constructor)) void libGlobCCtor(void)
{
	assert(libraryGlobalConstructorVariable == 12);
	libraryGlobalConstructorVariable = 43;
}

void testLibraryGlobalConstructors()
{
	assert(libraryGlobalConstructorVariable == 43);
}

/**
 * Tests thread-local variable from libc.
 */
void testLibraryThreadLocalFromLibc()
{
	assert(errno == 123);
	errno = 321;
	assert(errno == 321);
}

/**
 * Library test method called from the main executable.
 */
void libRuntimeTest(int someValue)
{
	testLibraryGlobalConstructors();
	testLibraryGlobalCppConstructors();
	testLibraryThreadLocalFromLibc();
	testLibraryThreadLocal();

	throw someValue * 2;
}
