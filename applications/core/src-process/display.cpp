#include "process.hpp"

static void processPrintSample(const ProcessSample& sample, const ProcessSample* old)
{
	char cpu[32] = "-", tid[16] = "-", memory[32] = "-";

	if(old && sample.sampledAt > old->sampledAt && sample.info.cpu_time >= old->info.cpu_time)
	{
		uint64_t elapsed = sample.sampledAt - old->sampledAt;
		uint64_t used = sample.info.cpu_time - old->info.cpu_time;
		uint64_t tenths = used / elapsed * 1000 + used % elapsed * 1000 / elapsed;
		snprintf(cpu, sizeof(cpu), "%llu.%llu", static_cast<unsigned long long>(tenths / 10),
				 static_cast<unsigned long long>(tenths % 10));
	}
	if(sample.thread)
		snprintf(tid, sizeof(tid), "%i", sample.info.id);
	else
		snprintf(memory, sizeof(memory), "%llu", static_cast<unsigned long long>(sample.info.memory_used / 1024));
	printf("%5i %5s %7s %12llu %10s %s\n", sample.info.parent, tid, cpu,
		   static_cast<unsigned long long>(sample.info.cpu_time / 1000000ULL), memory,
		   sample.info.identifier[0]	? sample.info.identifier
		   : sample.info.source_path[0] ? sample.info.source_path
										: "-");
}

int processList(bool threads)
{
	std::vector<ProcessSample> before, after;
	if(!processSnapshot(threads, before))
	{
		fprintf(stderr, "process: cannot query processes\n");
		return 1;
	}
	g_sleep(250);
	if(!processSnapshot(threads, after))
	{
		fprintf(stderr, "process: cannot query processes\n");
		return 1;
	}
	std::map<std::pair<bool, g_tid>, const ProcessSample*> previous;
	for(const auto& sample: before)
		previous[{sample.thread, sample.info.id}] = &sample;
	printf("%5s %5s %7s %12s %10s %s\n", "PID", "TID", "CPU%", "CPU-MS", "RSS-KiB", "NAME");
	for(const auto& sample: after)
	{
		auto old = previous.find({sample.thread, sample.info.id});
		processPrintSample(sample, old == previous.end() ? nullptr : old->second);
	}
	return 0;
}
