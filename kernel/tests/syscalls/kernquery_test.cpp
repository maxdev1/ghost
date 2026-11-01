#include "support/fixture.hpp"

namespace
{
#define GHOST_API_STDINT
typedef uint8_t g_bool;
#include <ghost/kernquery/callstructs.h>

struct g_task;
struct g_task_entry { g_task* task; g_task_entry* next; };
struct g_process
{
	g_pid id;
	g_mutex lock;
	uint64_t cpuTime;
	g_task_entry* tasks;
	g_physical_address pageSpace;
	struct { const char* executablePath; } environment;
};
struct g_task
{
	g_process* process;
	g_tid id;
	g_task_status status;
	g_task_type type;
	g_mutex lock;
	uint64_t cpuTime;
	unsigned references;
};
std::vector<g_task*> registry;
auto taskGlobalMap = &registry;
struct QueryIterator { size_t position; struct { g_task* value; } entry; };
QueryIterator hashmapIteratorStart(std::vector<g_task*>*) { return {}; }
bool hashmapIteratorHasNext(QueryIterator* it) { return it->position < registry.size(); }
auto hashmapIteratorNext(QueryIterator* it) -> decltype(&it->entry)
{
	it->entry.value = registry[it->position++];
	return &it->entry;
}
void hashmapIteratorEnd(QueryIterator*) {}
unsigned hashmapSize(std::vector<g_task*>*) { return registry.size(); }
g_task* taskingAcquireTask(g_tid id)
{
	for(auto task : registry)
		if(task->id == id) { ++task->references; return task; }
	return nullptr;
}
void taskingReleaseTask(g_task* task)
{
	if(task) { EXPECT_GT(task->references, 0U); --task->references; }
}
unsigned flushes;
uint64_t schedulerGetCpuTime(g_task* task)
{
	EXPECT_GT(task->process->lock.depth, 0);
	EXPECT_EQ(0, task->lock.depth);
	++flushes;
	return task->cpuTime;
}
uint64_t pagingCountResidentUserBytes(g_physical_address directory)
{
	EXPECT_EQ(0x1000U, directory);
	return 12288;
}
void taskingDirectoryCopyIdentifier(g_tid, char* buffer, size_t) { strcpy(buffer, "test-task"); }
int stringCopy(char* out, const char* in) { strcpy(out, in); return strlen(in); }
#include "production/syscallKernQuery.inc"

class KernQuery : public ::testing::Test
{
protected:
	g_process process{};
	g_task main{}, worker{};
	g_task_entry mainEntry{}, workerEntry{};
	void SetUp() override
	{
		process.id = 5;
		process.cpuTime = 900000000;
		process.pageSpace = 0x1000;
		process.environment.executablePath = "/test.bin";
		main.process = worker.process = &process;
		main.id = 5; worker.id = 6;
		main.status = worker.status = G_TASK_STATUS_RUNNING;
		main.cpuTime = 300000000; worker.cpuTime = 100000000;
		mainEntry = {&main, &workerEntry};
		workerEntry = {&worker, nullptr};
		process.tasks = &mainEntry;
		registry = {&main, &worker};
		flushes = 0;
	}
	void TearDown() override
	{
		EXPECT_EQ(0U, main.references);
		EXPECT_EQ(0U, worker.references);
		EXPECT_EQ(0, process.lock.depth);
		EXPECT_EQ(0, main.lock.depth);
		EXPECT_EQ(0, worker.lock.depth);
	}
	g_kernquery_status query(uint16_t command, g_kernquery_task_get_data& info)
	{
		g_syscall_kernquery call{command, reinterpret_cast<uint8_t*>(&info), -1};
		syscallKernQuery(&main, &call);
		return call.status;
	}
};

TEST_F(KernQuery, ProcessTotalsIncludeExitedThreadsAndShareResidentMemory)
{
	g_kernquery_task_get_data info{};
	info.id = 5;
	EXPECT_EQ(G_KERNQUERY_STATUS_SUCCESSFUL, query(G_KERNQUERY_PROCESS_GET_BY_ID, info));
	EXPECT_TRUE(info.found);
	EXPECT_EQ(900000000U, uint64_t(info.cpu_time));
	EXPECT_EQ(12288U, uint64_t(info.memory_used));
	EXPECT_EQ(2U, flushes);
	EXPECT_STREQ("test-task", info.identifier);
	EXPECT_STREQ("/test.bin", info.source_path);
	info.id = 6;
	EXPECT_EQ(G_KERNQUERY_STATUS_SUCCESSFUL, query(G_KERNQUERY_TASK_GET_BY_ID, info));
	EXPECT_EQ(100000000U, uint64_t(info.cpu_time));
	EXPECT_EQ(5, int(info.parent));
	EXPECT_EQ(12288U, uint64_t(info.memory_used));
}

TEST_F(KernQuery, RejectsThreadIdsAsProcessIdsAndSkipsDeadOrMissingTasks)
{
	g_kernquery_task_get_data info{};
	for(int id : {6, 999})
	{
		info.id = id;
		EXPECT_EQ(G_KERNQUERY_STATUS_UNKNOWN_ID, query(G_KERNQUERY_PROCESS_GET_BY_ID, info));
		EXPECT_FALSE(info.found);
		EXPECT_EQ(-1, int(info.id));
	}
	worker.status = G_TASK_STATUS_DEAD;
	info.id = 6;
	EXPECT_EQ(G_KERNQUERY_STATUS_UNKNOWN_ID, query(G_KERNQUERY_TASK_GET_BY_ID, info));
	EXPECT_FALSE(info.found);
	EXPECT_EQ(0U, flushes);
}

TEST_F(KernQuery, CountAndListRespectCallerCapacity)
{
	g_kernquery_task_count_data count{};
	g_syscall_kernquery call{G_KERNQUERY_TASK_COUNT, reinterpret_cast<uint8_t*>(&count), -1};
	syscallKernQuery(&main, &call);
	EXPECT_EQ(2U, unsigned(count.count));
	g_tid ids[2]{-1, -1};
	g_kernquery_task_list_data list{ids, 1, 0};
	call.command = G_KERNQUERY_TASK_LIST;
	call.buffer = reinterpret_cast<uint8_t*>(&list);
	syscallKernQuery(&main, &call);
	EXPECT_EQ(1U, unsigned(list.filled_ids));
	EXPECT_EQ(5, ids[0]);
	EXPECT_EQ(-1, ids[1]);
}
}
