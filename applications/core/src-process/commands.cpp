#include "process.hpp"

static bool processParseId(const char* text, g_pid& id)
{
	uint64_t value;
	if(!coreParseUnsigned(text, &value, INT32_MAX))
		return false;
	id = value;
	return true;
}

static int processKill(g_pid id)
{
	g_kernquery_task_get_data info{};
	auto query = processQuery(id, false, info);
	if(query != G_KERNQUERY_STATUS_SUCCESSFUL)
	{
		if(query == G_KERNQUERY_STATUS_UNKNOWN_ID)
			fprintf(stderr, "process: process %i does not exist\n", id);
		else
			fprintf(stderr, "process: cannot query process %i\n", id);
		return 1;
	}
	auto status = g_kill(id);
	if(status == G_KILL_STATUS_SUCCESSFUL)
	{
		printf("Requested termination of process %i.\n", id);
		return 0;
	}
	if(status == G_KILL_STATUS_NOT_FOUND)
		fprintf(stderr, "process: process %i does not exist\n", id);
	else
		fprintf(stderr, "process: cannot terminate process %i (status %i)\n", id, int(status));
	return 1;
}

int processRun(int argc, char** argv)
{
	const char* usage = "Usage: process list [--threads]\n       process ID kill\n";
	if(argc == 2 && coreArgumentIsHelp(argv[1]))
	{
		printf("%sCPU%% is sampled over 250 ms; 100%% represents one CPU. CPU-MS is cumulative.\n"
			   "RSS-KiB is resident user memory; threads share the process's memory.\n",
			   usage);
		return 0;
	}
	if((argc == 2 || (argc == 3 && !strcmp(argv[2], "--threads"))) && !strcmp(argv[1], "list"))
		return processList(argc == 3);
	if(argc == 3 && !strcmp(argv[2], "kill"))
	{
		g_pid id;
		if(!processParseId(argv[1], id))
		{
			fprintf(stderr, "process: invalid process ID '%s'\n", argv[1]);
			return 1;
		}
		return processKill(id);
	}
	fprintf(stderr, "%s", usage);
	return 1;
}
