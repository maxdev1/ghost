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

#include "tester.hpp"
#include <ghost.h>
#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>


int main(int argc, char** argv)
{
	klog("Starting test suite...");

	/* Run basic tests */
	test_result_t result;
	result += runStdioTest();
	result += runMessageTest();
	result += runThreadTests();

	klog("Test suite finished: %i successful, %i failed", result.successful, result.failed);

	/* Perform runtime tests */
	klog("Starting runtime tests");
	g_spawn("/applications/runtimetest.bin", "", "", G_SECURITY_LEVEL_APPLICATION);
	g_spawn("/applications/runtimetest-static.bin", "", "", G_SECURITY_LEVEL_APPLICATION);

	/* Restart test suite */
	g_sleep(3000);
	klog("Test-suite is restarting itself...");
	g_spawn("/applications/tester.bin", "-respawned", "/", G_SECURITY_LEVEL_APPLICATION);
}

/**
 * Performs a few threading-related tests.
 */
int testLocalSetAfterJoin = 0;

void testThreadRoutine()
{
	/* Register in task directory */
	g_task_register_id("test-routine");

	/* Wait a little before finishing */
	for(int i = 0; i < 5; i++) {
		g_sleep(100);
	}
	testLocalSetAfterJoin = 25;
}

test_result_t runThreadTests()
{
	g_tid tid = g_create_thread((void*) testThreadRoutine);

	/* Check if task directory registration has worked */
	g_sleep(100);
	g_tid registeredTid = g_task_get_id("test-routine");
	assert(tid == registeredTid);

	/* Test joining */
	assert(testLocalSetAfterJoin == 0);
	g_join(tid);
	assert(testLocalSetAfterJoin == 25);

	TEST_SUCCESSFUL;
}
