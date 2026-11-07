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
#include "production/time_callstructs.inc"

struct g_task {};
struct g_clock_local { uint64_t time; };

static g_clock_local localClocks[2];
static bool hasHpet;
static uint64_t referenceCounter;
static uint64_t hpetNanos;
static int64_t wallClockUnixSeconds;
static uint32_t wallClockProcessor;
static uint64_t wallClockStartNanos;

uint32_t processorGetCurrentId() { return 1; }
g_clock_local* clockGetLocal() { return &localClocks[1]; }
g_clock_local* _clockGetLocalForProcessor(uint32_t processor) { return &localClocks[processor]; }
bool hpetIsAvailable() { return hasHpet; }
uint64_t hpetGetNanos() { return hpetNanos; }
uint64_t processorReadTsc() { return 0; }
void processorReadMsr(uint32_t index, uint32_t* low, uint32_t* high)
{
	EXPECT_EQ(0x40000020u, index);
	*low = referenceCounter; *high = referenceCounter >> 32;
}

#include "production/time_syscalls.inc"

TEST(TimeSyscalls, KeepsElapsedTimerUnitsSeparateFromUnixSecondsOnBothClockSources)
{
	hypervisorClock = false;
	for(bool hpet: {false, true})
	{
		hasHpet = hpet;
		hpetNanos = 3500000000ULL;
		localClocks[0].time = 3500;
		localClocks[1].time = 1234;
		wallClockUnixSeconds = 1793810000;
		wallClockProcessor = 0;
		wallClockStartNanos = 1000000000ULL;

		g_syscall_millis millis{};
		syscallGetMilliseconds(nullptr, &millis);
		EXPECT_EQ(uint64_t(millis.millis), hpet ? 2500u : 3500u);

		g_syscall_nanos nanos{};
		syscallGetNanoseconds(nullptr, &nanos);
		EXPECT_EQ(uint64_t(nanos.nanos), hpet ? hpetNanos : 3500000000ULL);

		g_syscall_time unixTime{};
		syscallGetTime(nullptr, &unixTime);
		EXPECT_EQ(int64_t(unixTime.unixSeconds), 1793810002);
	}
}

TEST(TimeSyscalls, ReportsMissingClockBackwardTimerAndTimestampOverflow)
{
	hypervisorClock = false;
	hasHpet = true;
	hpetNanos = 5000000000ULL;
	wallClockStartNanos = 0;
	wallClockUnixSeconds = 0;

	g_syscall_time result{};
	syscallGetTime(nullptr, &result);
	EXPECT_EQ(int64_t(result.unixSeconds), -1);

	wallClockUnixSeconds = 1793810000;
	wallClockStartNanos = hpetNanos + 1;
	syscallGetTime(nullptr, &result);
	EXPECT_EQ(int64_t(result.unixSeconds), -1);

	wallClockUnixSeconds = INT64_MAX;
	wallClockStartNanos = 0;
	syscallGetTime(nullptr, &result);
	EXPECT_EQ(int64_t(result.unixSeconds), -1);
}

TEST(TimeSyscalls, UsesTheFullHypervCounterWith100NanosecondUnits)
{
	hypervisorClock = true;
	hasHpet = true;
	hpetNanos = 0;
	referenceCounter = UINT64_C(0x100000000) + 123;
	wallClockStartNanos = referenceCounter * 100;
	wallClockUnixSeconds = 1793810000;
	referenceCounter += 12500000;
	g_syscall_millis millis{};
	syscallGetMilliseconds(nullptr, &millis);
	EXPECT_EQ(1250u, millis.millis);
	g_syscall_nanos nanos{};
	syscallGetNanoseconds(nullptr, &nanos);
	EXPECT_EQ(referenceCounter * 100, nanos.nanos);
	g_syscall_time unixTime{};
	syscallGetTime(nullptr, &unixTime);
	EXPECT_EQ(1793810001, unixTime.unixSeconds);
	hypervisorClock = false;
}
}
