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
using g_tid = int;
struct g_tasking_local { uint32_t processor; } assignments[2] = {{0}, {1}};
struct g_task
{
    g_tid id;
    g_tasking_local* assignment;
    struct { g_task* next; uint64_t wakeTime; bool active; } timeoutWait;
    bool woke;
};
#include "production/clock_hpp.inc"
#include "production/timer_configuration.inc"
static g_task tasks[3];
static uint32_t processor;
static bool hasHpet, hasLapic, hasHyperv, hasReferencePage, allocateReferencePage, changeReferenceSequence;
static uint64_t referenceCounter;
static unsigned hpetReads, referenceReads, tscReads, controlWrites;
static uint64_t tsc, writtenControl;
static uint32_t vmwareFrequency;
static constexpr uint32_t G_PAGE_SIZE = 4096;
alignas(4096) static uint8_t referenceStorage[G_PAGE_SIZE];
static uint64_t nanos, timerDelay;
static bool timerIdle;
static unsigned timerCalls;
static std::vector<uint32_t> notifications;
uint32_t processorGetCurrentId() { return processor; }
uint32_t processorGetNumberOfProcessors() { return 2; }
uint32_t processorGetVmwareTscKhz() { return vmwareFrequency; }
bool hpetIsAvailable() { return hasHpet; }
uint64_t hpetGetNanos() { ++hpetReads; return nanos; }
bool processorHasHypervFeature(uint32_t feature)
{
    return hasHyperv && (feature == (1u << 1) || (hasReferencePage && feature == (1u << 9)));
}
void processorReadMsr(uint32_t index, uint32_t* low, uint32_t* high)
{
    if(index == 0x40000021) { *low = 0xa00; *high = 0; return; }
    EXPECT_EQ(0x40000020u, index);
    ++referenceReads;
    *low = referenceCounter; *high = referenceCounter >> 32;
}
void processorWriteMsr(uint32_t, uint32_t, uint32_t);
uint64_t processorReadTsc();
g_virtual_address memoryAllocateKernel(int32_t pages)
{
    EXPECT_EQ(1u, pages);
    return allocateReferencePage ? (g_virtual_address) referenceStorage : 0;
}
g_physical_address pagingVirtualToPhysical(g_virtual_address page)
{
    EXPECT_EQ((g_virtual_address) referenceStorage, page);
    return 0x12345000;
}
void* memorySetBytes(void* address, uint8_t value, int32_t length) { return std::memset(address, value, length); }
void* heapAllocate(size_t size) { return std::calloc(1, size); }
g_task* taskingAcquireTask(g_tid id) { return id >= 0 && id < 3 ? &tasks[id] : nullptr; }
void taskingReleaseTask(g_task*) {}
void taskingWake(g_task* task) { task->woke = true; }
bool lapicIsAvailable() { return hasLapic; }
void lapicConfigureTimer(bool idle, uint64_t delay)
{
    ++timerCalls; timerIdle = idle; timerDelay = delay;
}
void schedulerRequestReschedule(uint32_t destination);
#include "production/clock_cpp.inc"
void processorWriteMsr(uint32_t index, uint32_t low, uint32_t high)
{
    EXPECT_EQ(0x40000021u, index);
    ++controlWrites;
    writtenControl = ((uint64_t) high << 32) | low;
    auto page = (g_clock_reference_page*) referenceStorage;
    page->sequence = 1; page->scale = UINT64_C(1) << 63; page->offset = 0;
}
uint64_t processorReadTsc()
{
    ++tscReads;
    if(changeReferenceSequence)
    {
        changeReferenceSequence = false;
        ++referencePage->sequence;
        referencePage->offset = 1000;
    }
    return tsc;
}
void schedulerRequestReschedule(uint32_t destination)
{
    for(unsigned i = 0; i < 2; ++i) EXPECT_EQ(0, locals[i].lock.depth);
    notifications.push_back(destination);
}
static void clockTestReset(bool hpet, bool hyperv = false, bool page = false, bool allocate = true,
                           uint32_t vmwareKhz = 0)
{
    if(locals) std::free(locals);
    locals = nullptr;
    tasks[0] = {0, &assignments[0], {}, false};
    tasks[1] = {1, &assignments[1], {}, false};
    tasks[2] = {2, &assignments[1], {}, false};
    processor = 0; hasHpet = hpet; hasLapic = true; nanos = 1000000000ULL;
    hasHyperv = hyperv; referenceCounter = nanos / 100; hpetReads = 0;
    hasReferencePage = page; allocateReferencePage = allocate; changeReferenceSequence = false;
    referenceReads = 0; tscReads = 0; controlWrites = 0; writtenControl = 0; tsc = 20000000;
    vmwareFrequency = vmwareKhz;
    notifications.clear(); timerCalls = 0;
    clockInitializeReference();
    clockInitialize(1000);
}
TEST(ClockIdle, VmwareTscKeepsElapsedTimeAcrossMissingTicksAndSmallCoreOffsets)
{
    clockTestReset(false, false, false, false, 2500000);
    clockWaitForTime(1, 5000);
    tsc += 12500000000ULL;
    EXPECT_EQ(5000u, clockGetMilliseconds());
    processor = 1;
    tsc -= 750; // This vCPU is 300 ns behind the last reader.
    EXPECT_EQ(5000000000ULL, clockGetNanoseconds(1));
    uint64_t sample;
    ASSERT_TRUE(clockReadCalibrationNanos(&sample));
    EXPECT_EQ(4999999700ULL, sample);
    clockRefreshWaiters();
    EXPECT_TRUE(tasks[1].woke);
    EXPECT_EQ(5000u, locals[1].time);
    tsc += 2500750;
    clockUpdate();
    EXPECT_EQ(5001u, locals[1].time);
    EXPECT_EQ(1005, clockGetWallTime());
}
TEST(ClockIdle, VmwareTscConversionRetainsLongUptimeAndFractionalKilohertzUnits)
{
    clockTestReset(false, false, false, false, 2500001);
    const uint64_t milliseconds = 365ULL * 24 * 60 * 60 * 1000;
    tsc += milliseconds * 2500001 + 123456;
    EXPECT_EQ(milliseconds * 1000000 + 49382, clockGetNanoseconds(0));
    EXPECT_EQ(milliseconds, clockGetMilliseconds());
}
TEST(ClockIdle, CalibrationRequiresAReferenceAndPreservesSourcePriority)
{
    uint64_t sample = 123;
    clockTestReset(false);
    EXPECT_FALSE(clockReadCalibrationNanos(&sample));
    EXPECT_EQ(123u, sample);
    clockTestReset(true, false, false, false, 2500000);
    EXPECT_TRUE(clockReadCalibrationNanos(&sample));
    EXPECT_EQ(nanos, sample);
    EXPECT_EQ(0u, tscReads);
    clockTestReset(true, true, false, false, 2500000);
    ++referenceCounter;
    EXPECT_TRUE(clockReadCalibrationNanos(&sample));
    EXPECT_EQ(referenceCounter * 100, sample);
    EXPECT_EQ(0u, hpetReads);
    EXPECT_EQ(0u, tscReads);
}
TEST(ClockIdle, FallbackClockIsSharedAndSecondaryTicksCannotAdvanceIt)
{
    clockTestReset(false);
    clockUpdate();
    EXPECT_EQ(1u, clockGetMilliseconds());
    processor = 1;
    for(unsigned i = 0; i < 100; ++i) clockUpdate();
    EXPECT_EQ(1u, clockGetMilliseconds());
    processor = 0;
    for(unsigned i = 0; i < 8; ++i) clockUpdate();
    processor = 1;
    EXPECT_EQ(9u, clockGetMilliseconds());
    EXPECT_EQ(9000000u, clockGetNanoseconds(0));
    EXPECT_EQ(clockGetNanoseconds(0), clockGetNanoseconds(1));
}
TEST(ClockIdle, HardwareTimeAdvancesWhileIdleAndRefreshDoesNotDoubleCount)
{
    clockTestReset(true);
    processor = 1; nanos += 1234567890ULL;
    EXPECT_EQ(1234u, clockGetMilliseconds());
    clockRefreshWaiters();
    EXPECT_EQ(1234u, locals[1].time);
    nanos += 4000000ULL; clockUpdate();
    EXPECT_EQ(1238u, locals[1].time);
    processor = 0; EXPECT_EQ(1238u, clockGetMilliseconds());
}
TEST(ClockIdle, HypervCounterAdvancesIdleDeadlinesWithoutReadingHpet)
{
    for(bool hpet : {false, true})
    {
        clockTestReset(hpet, true);
        processor = 1;
        referenceCounter += 12345678;
        EXPECT_EQ(1234u, clockGetMilliseconds());
        EXPECT_EQ(referenceCounter * 100, clockGetNanoseconds(0));
        EXPECT_EQ(clockGetNanoseconds(0), clockGetNanoseconds(1));
        clockRefreshWaiters();
        EXPECT_EQ(1234u, locals[1].time);
        clockWaitForTime(1, 1250);
        clockConfigureLocalTimer(true);
        EXPECT_EQ(16u, timerDelay);
        referenceCounter += 200000;
        clockRefreshWaiters();
        EXPECT_TRUE(tasks[1].woke);
        EXPECT_FALSE(tasks[1].timeoutWait.active);
        EXPECT_EQ(1254u, locals[1].time);
        processor = 0;
        clockUpdate();
        EXPECT_EQ(1254u, clockGetMilliseconds());
        EXPECT_EQ(0u, hpetReads);
    }
}
TEST(ClockIdle, SharedReferencePagePreservesReservedBitsAndUsesSignedOffset)
{
    clockTestReset(true, true, true);
    ASSERT_NE(nullptr, referencePage);
    EXPECT_EQ(UINT64_C(0x12345a01), writtenControl);
    EXPECT_EQ(1u, controlWrites);
    referencePage->offset = -123; tsc += 246; ++referencePage->sequence;
    EXPECT_EQ(0u, clockGetMilliseconds());
    tsc += 20000000;
    EXPECT_EQ(1000u, clockGetMilliseconds());
    processor = 1;
    clockWaitForTime(1, 1005); clockConfigureLocalTimer(true);
    EXPECT_EQ(5u, timerDelay);
    tsc += 100000; clockRefreshWaiters();
    EXPECT_TRUE(tasks[1].woke);
    EXPECT_EQ(0u, hpetReads);
    EXPECT_EQ(0u, referenceReads);
}
TEST(ClockIdle, SharedReferencePageRetriesUpdatesAndFallsBackWhenUnavailable)
{
    clockTestReset(false, true, true);
    tscReads = 0; changeReferenceSequence = true;
    EXPECT_EQ(1000100000u, clockGetNanoseconds(0));
    EXPECT_EQ(2u, tscReads);
    EXPECT_EQ(0u, referenceReads);
    referencePage->sequence = 0; referenceCounter += 1250000; tscReads = 0;
    EXPECT_EQ(125u, clockGetMilliseconds());
    EXPECT_EQ(1u, referenceReads);
    EXPECT_EQ(0u, tscReads);
}
TEST(ClockIdle, ReferencePageAllocationFailureKeepsTheAdvertisedCounter)
{
    clockTestReset(false, true, true, false);
    EXPECT_EQ(nullptr, referencePage);
    EXPECT_EQ(0u, controlWrites);
    referenceCounter += 10000000;
    EXPECT_EQ(1000u, clockGetMilliseconds());
    EXPECT_EQ(0u, tscReads);
}
TEST(ClockIdle, ProgramsTheNearestDeadlineAndNotifiesAfterRemoteQueueChanges)
{
    clockTestReset(false);
    for(unsigned i = 0; i < 100; ++i) clockUpdate();
    clockWaitForTime(1, 150);
    ASSERT_EQ(1u, notifications.size()); EXPECT_EQ(1u, notifications.back());
    processor = 1; clockConfigureLocalTimer(true);
    EXPECT_TRUE(timerIdle); EXPECT_EQ(50u, timerDelay);
    processor = 0;
    for(unsigned i = 0; i < 20; ++i) clockUpdate();
    clockWaitForTime(2, 125);
    processor = 1; clockConfigureLocalTimer(true);
    EXPECT_EQ(5u, timerDelay);
    processor = 0; clockUnwaitForTime(&tasks[2]);
    processor = 1; clockConfigureLocalTimer(true);
    EXPECT_EQ(30u, timerDelay);
    clockUnwaitForTime(&tasks[1]); clockConfigureLocalTimer(true);
    EXPECT_TRUE(timerIdle); EXPECT_EQ(0u, timerDelay);
    clockConfigureLocalTimer(false); EXPECT_FALSE(timerIdle);
}
TEST(ClockIdle, BootstrapKeepsTimeAndDueWaitersWakeAfterAnIdleOneShot)
{
    clockTestReset(false);
    clockConfigureLocalTimer(true); EXPECT_FALSE(timerIdle);
    processor = 1; clockWaitForTime(1, 5); clockConfigureLocalTimer(true);
    EXPECT_TRUE(timerIdle); EXPECT_EQ(5u, timerDelay);
    processor = 0; for(unsigned i = 0; i < 5; ++i) clockUpdate();
    processor = 1; clockRefreshWaiters();
    EXPECT_TRUE(tasks[1].woke); EXPECT_FALSE(tasks[1].timeoutWait.active);
    EXPECT_EQ(nullptr, locals[1].waiters); EXPECT_EQ(5u, clockGetMilliseconds());
    EXPECT_TRUE(clockHasTimedOut(1));
    hasLapic = false; timerCalls = 0; clockConfigureLocalTimer(true);
    EXPECT_EQ(0u, timerCalls);
    std::free(locals); locals = nullptr;
}
}
