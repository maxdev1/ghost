/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ghost.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "../../libruntimetest/inc/libruntimetest.h"

/**
 * Test for C++-style global constructors.
 */
class GlobalConstructorTestClass
{
public:
	int field = 5;
	GlobalConstructorTestClass()
	{
		assert(field == 5);
		field = 12;
	}
};

GlobalConstructorTestClass localGlobalCppConstructorTest;

void testGlobalCppConstructors()
{
	assert(localGlobalCppConstructorTest.field == 12);
}

/**
 * Tests for C-style global constructors.
 */
int localGlobalConstructorVariable = 6432;

__attribute__ ((constructor)) void globCCtor(void)
{
	assert(localGlobalConstructorVariable == 6432);
	localGlobalConstructorVariable = 8143;
}

void testGlobalConstructors()
{
	assert(localGlobalConstructorVariable == 8143);
}

/**
 * Tests for local throwing of exceptions.
 */
void throwLocalException()
{
	throw 89;
}

void testLocalExceptions()
{
	try
	{
		throwLocalException();
	} catch(int y)
	{
		assert(y == 89);
	}
}

/**
 * Tests local thread-local variables.
 */
__thread int threadLocalVariable = 5;

void testThreadLocal()
{	
	assert(threadLocalVariable == 5);
	threadLocalVariable = 6;
	assert(threadLocalVariable == 6);
}

/**
 * Tests thread-local variable coming from libc.
 */
void testThreadLocalFromLibc()
{
	assert(errno == 0);
	errno = 123;
	assert(errno == 123);
}


/**
 * The library sets errno to 321.
 */
void testErrnoSetFromLibrary()
{
	assert(errno == 321);
}

void testCallLibrary()
{
	try {
		libRuntimeTest(25);
	} catch(int x) {
		assert(x == 50);
	}
}

int main(int argc, char** argv)
{
	testGlobalConstructors();
	testGlobalCppConstructors();
	testThreadLocal();
	testThreadLocalFromLibc();
	testLocalExceptions();
	testCallLibrary();
	testErrnoSetFromLibrary();
	g_log("Runtime test suite finished successfully");
}
