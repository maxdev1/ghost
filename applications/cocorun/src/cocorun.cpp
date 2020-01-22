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

#include "../libcoconut/inc/coconut.h"

class GlobCtorTest {
public:
	int x = 5;
	GlobCtorTest() {
		assert(x == 5);
		x = 12;
	}
};

GlobCtorTest ctortest;

int cctor = 0;
__attribute__ ((constructor)) void globCCtor(void)
{
	cctor = 1;
}

__thread int foo = 5;

int main(int argc, char** argv)
{
	g_log("Starting global ctor tests...");

	assert(foo == 5);
	foo = 6;
	assert(foo == 6);

	assert(errno == 0);
	errno = 123;
	assert(errno == 123);

	assert(ctortest.x == 12);

	try {
		coconutThrow(25);
	} catch(int x) {
		g_log("catch");
		assert(x == 50);
	}

	assert(errno == 321);

	g_log("All assertions finished successfully!");
}
