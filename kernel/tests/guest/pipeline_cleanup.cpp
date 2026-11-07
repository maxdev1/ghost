/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

static void pipelineTestRequire(bool condition, const char* message)
{
	if(!condition)
	{
		klog("pipeline-cleanup-test: FAIL %s", message);
		g_exit(1);
	}
}

static g_pid pipelineTestSpawn(const char* program, const char* arguments, g_fd input, g_fd output)
{
	g_fd stdioIn[3] = {input, output, G_FD_NONE};
	g_fd stdioOut[3];
	g_pid process;
	pipelineTestRequire(g_spawn_poi(program, arguments, "/", G_SECURITY_LEVEL_APPLICATION,
		&process, stdioOut, stdioIn) == G_SPAWN_STATUS_SUCCESSFUL, "spawn");
	return process;
}

int main()
{
	static const char input[] = "Ghost pipeline\nImmediate EOF\n";
	static const char expected[] = "enilepip tsohG\nFOE etaidemmI\n";
	int file = open("/pipeline-input", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	pipelineTestRequire(file >= 0, "create input");
	pipelineTestRequire(write(file, input, sizeof(input) - 1) == sizeof(input) - 1, "write input");
	close(file);

	for(int trial = 0; trial < 4; ++trial)
	{
		g_sleep(37 + trial * 43);
		auto start = g_millis();
		g_fd firstWrite, firstRead, secondWrite, secondRead;
		pipelineTestRequire(g_pipe(&firstWrite, &firstRead) == G_FS_PIPE_SUCCESSFUL, "first pipe");
		pipelineTestRequire(g_pipe(&secondWrite, &secondRead) == G_FS_PIPE_SUCCESSFUL, "second pipe");
		auto reader = pipelineTestSpawn("/applications/read.bin", "/pipeline-input", G_FD_NONE, firstWrite);
		g_close(firstWrite);
		auto reverse = pipelineTestSpawn("/applications/reverse.bin", "", firstRead, secondWrite);
		g_close(firstRead);
		g_close(secondWrite);
		auto writer = pipelineTestSpawn("/applications/write.bin", "/pipeline-output", secondRead, G_FD_NONE);
		g_close(secondRead);
		g_join(writer);
		g_join(reverse);
		g_join(reader);
		auto elapsed = g_millis() - start;
		klog("pipeline-cleanup-test: trial %i completed in %i ms", trial, (int) elapsed);

		char output[sizeof(expected)] = {};
		file = open("/pipeline-output", O_RDONLY);
		pipelineTestRequire(file >= 0, "open output");
		pipelineTestRequire(read(file, output, sizeof(output)) == sizeof(expected) - 1, "output length");
		close(file);
		pipelineTestRequire(!memcmp(output, expected, sizeof(expected) - 1), "output contents");
		pipelineTestRequire(elapsed < 2000, "pipeline waited for periodic cleanup");
	}

	klog("pipeline-cleanup-test: PASS prompt EOF across repeated pipelines");
	return 0;
}
