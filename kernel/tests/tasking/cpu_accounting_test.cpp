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
	struct { g_task* current; uint64_t lastAccountingTime; g_task* idleTask; } scheduling;
};

uint64_t times[2];
uint64_t clockGetNanoseconds(uint32_t processor) { return times[processor]; }
std::vector<g_tasking_local*> processors;
unsigned processorGetNumberOfProcessors() { return processors.size(); }
g_tasking_local* taskingGetLocalForProcessor(uint32_t processor) { return processors[processor]; }
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

TEST(CpuAccounting, SystemCountersSeparateIdleAndBusyAcrossProcessors)
{
	g_process process{};
	g_tasking_local first{}, second{};
	second.processor = 1;
	g_task idleFirst{&process, &first, 0}, idleSecond{&process, &second, 0};
	g_task busy{&process, &first, 0};
	first.scheduling.idleTask = &idleFirst;
	second.scheduling.idleTask = &idleSecond;
	processors = {&first, &second};
	times[0] = times[1] = 100;
	schedulerSetCurrent(&first, &idleFirst);
	schedulerSetCurrent(&second, &idleSecond);
	times[0] = times[1] = 200;
	schedulerSetCurrent(&first, &busy);
	times[0] = times[1] = 300;

	uint64_t total, idle;
	schedulerGetSystemCpuTime(&total, &idle);
	EXPECT_EQ(600U, total);
	EXPECT_EQ(300U, idle);
	EXPECT_EQ(100U, busy.cpuTime);
	EXPECT_EQ(0, first.lock.depth);
	EXPECT_EQ(0, second.lock.depth);

	times[0] = times[1] = 500;
	schedulerGetSystemCpuTime(&total, &idle);
	EXPECT_EQ(1000U, total);
	EXPECT_EQ(500U, idle);
	EXPECT_EQ(300U, busy.cpuTime);
	processors.clear();
}
}
