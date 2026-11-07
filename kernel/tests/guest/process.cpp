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
#include <string>
#include <unistd.h>

static uint32_t processTestStop;

static void processTestRequire(bool condition, const char* message)
{
	if(!condition)
	{
		klog("process-test: FAIL %s", message);
		g_exit(1);
	}
}

struct ProcessTestUsage
{
	uint64_t cpu_time;
	uint64_t memory_used;
};

static ProcessTestUsage processTestQuery(g_tid id, bool thread = false)
{
	g_kernquery_task_get_data info{};
	info.id = id;
	processTestRequire(g_kernquery(thread ? G_KERNQUERY_TASK_GET_BY_ID : G_KERNQUERY_PROCESS_GET_BY_ID,
	                              reinterpret_cast<uint8_t*>(&info)) == G_KERNQUERY_STATUS_SUCCESSFUL && info.found,
	                   "query usage");
	return {info.cpu_time, info.memory_used};
}

static void processTestBusy()
{
	g_task_register_name("process-test/busy");
	while(!__atomic_load_n(&processTestStop, __ATOMIC_ACQUIRE))
		asm volatile("" ::: "memory");
}

static void processTestSleeping()
{
	g_task_register_name("process-test/sleep");
	for(;;) g_sleep(1000);
}

static std::string processTestRead(const char* path)
{
	int file = open(path, O_RDONLY);
	processTestRequire(file >= 0, "open tool output");
	std::string result;
	char buffer[4096];
	ssize_t count;
	while((count = read(file, buffer, sizeof(buffer))) > 0) result.append(buffer, count);
	processTestRequire(count == 0, "read tool output");
	close(file);
	return result;
}

static std::string processTestTool(const std::string& arguments, bool success = true)
{
	int output = open("/process-test-out", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	int error = open("/process-test-err", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	processTestRequire(output >= 0 && error >= 0, "create tool output");
	g_fd streams[3]{G_FD_NONE, output, error};
	g_pid child;
	processTestRequire(g_spawn_poi("/applications/Process Manager.app/process.bin", arguments.c_str(), "/", G_SECURITY_LEVEL_APPLICATION,
	                              &child, nullptr, streams) == G_SPAWN_STATUS_SUCCESSFUL, "spawn process tool");
	close(output); close(error);
	g_join(child);
	processTestRequire(processTestRead("/process-test-err").empty() == success, "tool diagnostic");
	return processTestRead("/process-test-out");
}

int main(int argc, char** argv)
{
	if(argc == 2 && !strcmp(argv[1], "--target"))
	{
		processTestRequire(g_create_task(reinterpret_cast<void*>(processTestBusy)) != G_TID_NONE, "create busy thread");
		processTestRequire(g_create_task(reinterpret_cast<void*>(processTestSleeping)) != G_TID_NONE, "create sleeping thread");
		g_message_header message{};
		processTestRequire(g_receive_message(&message, sizeof(message)) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL,
		                   "receive worker stop request");
		__atomic_store_n(&processTestStop, 1, __ATOMIC_RELEASE);

		for(;;) g_sleep(1000);
	}

	klog("process-test: START");
	auto baseline = processTestQuery(g_get_pid());
	const size_t allocationSize = 32 * 4096;
	auto allocation = static_cast<volatile char*>(g_alloc_mem(allocationSize));
	processTestRequire(allocation != nullptr, "allocate memory");
	for(size_t i = 0; i < allocationSize; i += 4096) allocation[i] = 1;
	auto allocated = processTestQuery(g_get_pid());
	processTestRequire(allocated.memory_used >= baseline.memory_used + allocationSize, "resident allocation counted");
	g_unmap(const_cast<char*>(allocation));
	auto released = processTestQuery(g_get_pid());
	processTestRequire(released.memory_used + allocationSize <= allocated.memory_used, "unmapped memory excluded");
	klog("process-test: resident allocation/unmap PASS");

	g_pid target;
	processTestRequire(g_spawn_p("/applications/fenster.bin", "--target", "/", G_SECURITY_LEVEL_APPLICATION,
	                            &target) == G_SPAWN_STATUS_SUCCESSFUL, "spawn target");
	auto busy = g_task_await_by_name("process-test/busy");
	auto sleeping = g_task_await_by_name("process-test/sleep");
	auto processBefore = processTestQuery(target);
	auto busyBefore = processTestQuery(busy, true);
	auto sleepBefore = processTestQuery(sleeping, true);
	g_sleep(350);
	auto processAfter = processTestQuery(target);
	auto busyAfter = processTestQuery(busy, true);
	auto sleepAfter = processTestQuery(sleeping, true);
	processTestRequire(busyAfter.cpu_time > busyBefore.cpu_time + 20000000, "busy CPU time advances");
	processTestRequire(busyAfter.cpu_time - busyBefore.cpu_time > sleepAfter.cpu_time - sleepBefore.cpu_time,
	                   "sleeping thread uses less CPU");
	processTestRequire(processAfter.cpu_time > processBefore.cpu_time + 20000000, "process aggregates worker CPU");
	processTestRequire(busyAfter.memory_used == sleepAfter.memory_used, "threads share process memory");
	klog("process-test: CPU process/threads PASS");

	auto listing = processTestTool("list");
	processTestRequire(listing.find("RSS-KiB") != std::string::npos, "process list heading");
	listing = processTestTool(std::string("list") + char(G_CLIARGS_SEPARATOR) + "--threads");
	processTestRequire(listing.find("process-test/busy") != std::string::npos, "thread listing");
	processTestRequire(g_send_message(target, nullptr, 0) == G_MESSAGE_SEND_STATUS_SUCCESSFUL, "stop busy worker");
	g_join(busy);
	processTestRequire(processTestQuery(target).cpu_time >= processAfter.cpu_time, "exited thread CPU retained");
	processTestTool(std::string("0") + char(G_CLIARGS_SEPARATOR) + "kill", false);
	processTestTool(std::string("2147483647") + char(G_CLIARGS_SEPARATOR) + "kill", false);
	processTestTool(std::to_string(target) + char(G_CLIARGS_SEPARATOR) + "kill");
	g_join(target);
	for(auto id : {target, sleeping})
	{
		g_kernquery_task_get_data info{};
		info.id = id;
		processTestRequire(g_kernquery(G_KERNQUERY_TASK_GET_BY_ID, reinterpret_cast<uint8_t*>(&info)) ==
		                   G_KERNQUERY_STATUS_UNKNOWN_ID, "kill terminates all process threads");
	}

	klog("process-test: PASS list/threads/kill/CPU/resident-memory");
	return 0;
}
