#ifndef CORE_PROCESS
#define CORE_PROCESS

#include "../src-common/arguments.hpp"
#include <ghost/kernquery.h>
#include <ghost/tasks.h>
#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

struct ProcessSample
{
	g_kernquery_task_get_data info;
	uint64_t sampledAt;
	bool thread;
};

g_kernquery_status processQuery(g_tid id, bool thread, g_kernquery_task_get_data& info);
bool processSnapshot(bool threads, std::vector<ProcessSample>& samples);
int processList(bool threads);
int processRun(int argc, char** argv);

#endif
