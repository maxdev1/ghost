#include "support/fixture.hpp"

namespace
{
struct g_process { uint64_t cpuTime; };
struct g_tasking_local;
struct g_task { g_process* process; g_tasking_local* assignment; uint64_t cpuTime; };
struct g_tasking_local
{
	g_mutex lock;
	uint32_t processor;
	struct { g_task* current; uint64_t lastAccountingTime; } scheduling;
};
uint64_t times[2];
uint64_t clockGetNanoseconds(uint32_t processor) { return times[processor]; }
#include "production/schedulerAccounting.inc"

TEST(CpuAccounting, ChargesElapsedTimeInsteadOfCountingSchedules)
{
	g_process process{};
	g_tasking_local local{};
	g_task first{&process, &local, 0}, second{&process, &local, 0};
	times[0] = 100;
	schedulerSetCurrent(&local, &first);
	times[0] = 150;
	EXPECT_EQ(50U, schedulerGetCpuTime(&first));
	EXPECT_EQ(50U, schedulerGetCpuTime(&first));
	times[0] = 175;
	schedulerSetCurrent(&local, &second);
	times[0] = 300;
	EXPECT_EQ(125U, schedulerGetCpuTime(&second));
	EXPECT_EQ(75U, schedulerGetCpuTime(&first));
	EXPECT_EQ(200U, process.cpuTime);
	schedulerSetCurrent(&local, nullptr);
	times[0] = 900;
	EXPECT_EQ(75U, schedulerGetCpuTime(&first));
	EXPECT_EQ(200U, process.cpuTime);
}

TEST(CpuAccounting, AggregatesAcrossProcessorsAndRetainsExitedThreadTime)
{
	g_process process{};
	g_tasking_local first{}, second{};
	second.processor = 1;
	g_task main{&process, &first, 0}, worker{&process, &second, 0};
	times[0] = 100;
	times[1] = 500;
	schedulerSetCurrent(&first, &main);
	schedulerSetCurrent(&second, &worker);
	times[0] = 200;
	times[1] = 800;
	EXPECT_EQ(100U, schedulerGetCpuTime(&main));
	EXPECT_EQ(300U, schedulerGetCpuTime(&worker));
	EXPECT_EQ(400U, process.cpuTime);
	schedulerSetCurrent(&second, nullptr);
	worker = {};
	times[0] = 250;
	EXPECT_EQ(150U, schedulerGetCpuTime(&main));
	EXPECT_EQ(450U, process.cpuTime);
}

TEST(CpuAccounting, UnassignedTasksAndEqualTicksDoNotAccrueTime)
{
	g_process process{};
	g_tasking_local local{};
	g_task task{&process, nullptr, 0};
	EXPECT_EQ(0U, schedulerGetCpuTime(&task));
	task.assignment = &local;
	times[0] = 1000000;
	schedulerSetCurrent(&local, &task);
	for(int i = 0; i < 20; ++i)
		schedulerSetCurrent(&local, &task);
	EXPECT_EQ(0U, schedulerGetCpuTime(&task));
	times[0] = 2000000;
	EXPECT_EQ(1000000U, schedulerGetCpuTime(&task));
}
}
