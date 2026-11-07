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
#include "production/lapic_timer_hpp.inc"

static uint32_t currentProcessor;
static uint32_t calibratedTicks;
static std::vector<uint32_t> pitTickSamples;
static uint64_t apicBusHz;
static unsigned pitWaits;
static std::vector<uint64_t> calibrationTimes;
static size_t calibrationRead;
static bool interruptFlag;
static unsigned icrWaits;
static std::array<std::array<uint32_t, 64>, 256> registers;
struct g_lapic_test_write
{
    uint32_t processor;
    uint32_t reg;
    uint32_t value;
};
static std::vector<g_lapic_test_write> writes;
static std::map<uint32_t, std::array<uint32_t, 4>> cpuidLeaves;
static std::vector<uint32_t> cpuidQueries;
static std::vector<std::array<uint32_t, 3>> msrWrites;

void processorCpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    cpuidQueries.push_back(leaf);
    const auto values = cpuidLeaves[leaf];
    *eax = values[0]; *ebx = values[1]; *ecx = values[2]; *edx = values[3];
}

void processorWriteMsr(uint32_t index, uint32_t low, uint32_t high)
{
    msrWrites.push_back({index, low, high});
}

uint32_t processorGetCurrentId() { return currentProcessor; }
uint64_t processorGetApicFrequencyHz() { return apicBusHz; }
bool clockReadCalibrationNanos(uint64_t* nanos)
{
    if(calibrationRead == calibrationTimes.size())
        return false;
    *nanos = calibrationTimes.at(calibrationRead++);
    return true;
}
bool interruptsAreEnabled() { return interruptFlag; }
void interruptsEnable() { interruptFlag = true; }
void interruptsDisable() { interruptFlag = false; }

#include "production/lapic_interrupt_guard.inc"
#include "production/lapic_timer_state.inc"

uint32_t lapicRead(uint32_t reg)
{
    return registers.at(currentProcessor).at(reg / 16);
}

void lapicWrite(uint32_t reg, uint32_t value)
{
    EXPECT_FALSE(interruptFlag);
    registers.at(currentProcessor).at(reg / 16) = value;
    if(reg == APIC_REGISTER_TIMER_INITCNT)
        registers.at(currentProcessor).at(APIC_REGISTER_TIMER_CURRCNT / 16) = value;
    writes.push_back({currentProcessor, reg, value});
}

void pitPrepareSleep(uint32_t microseconds)
{
    EXPECT_EQ(10000u, microseconds);
    EXPECT_FALSE(interruptFlag);
}

void pitPerformSleep()
{
    const auto ticks = pitWaits < pitTickSamples.size() ? pitTickSamples[pitWaits] : calibratedTicks;
    ++pitWaits;
    registers.at(currentProcessor).at(APIC_REGISTER_TIMER_CURRCNT / 16) = UINT32_MAX - ticks;
}

void lapicWaitForIcrSend()
{
    EXPECT_FALSE(interruptFlag);
    ++icrWaits;
}

#include "production/lapic_timer.inc"
#include "production/lapic_ipi.inc"
#include "production/lapic_hypervisor.inc"

static void lapicTestReset()
{
    currentProcessor = 0;
    calibratedTicks = 100000;
    pitTickSamples.clear();
    apicBusHz = 0;
    pitWaits = 0;
    calibrationTimes.clear();
    calibrationRead = 0;
    interruptFlag = false;
    icrWaits = 0;
    registers = {};
    writes.clear();
    cpuidLeaves.clear();
    cpuidQueries.clear();
    msrWrites.clear();
    hypervisorEoi = false;
    for(auto& timer : timers)
        timer = {};
}

TEST(LapicTimer, CalibratesLocallyWithoutResettingAnActiveQuantum)
{
    lapicTestReset();
    lapicStartTimer();
    ASSERT_EQ(1000u, G_TIMER_FREQUENCY);
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(0x20u | APIC_LVT_TIMER_MODE_PERIODIC, lapicRead(APIC_REGISTER_LVT_TIMER));

    writes.clear();
    registers[0][APIC_REGISTER_TIMER_CURRCNT / 16] = 3500;
    lapicConfigureTimer(false, 0);
    lapicConfigureTimer(false, 500);
    EXPECT_TRUE(writes.empty());
    EXPECT_EQ(3500u, lapicRead(APIC_REGISTER_TIMER_CURRCNT));
}

TEST(LapicTimer, UsesAdvertisedBusFrequencyForPeriodicAndIdleTimers)
{
    lapicTestReset();
    apicBusHz = 1000000000;
    lapicStartTimer();
    EXPECT_EQ(62500u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    lapicConfigureTimer(true, 5);
    EXPECT_EQ(312500u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    currentProcessor = 1;
    apicBusHz = 66000000;
    lapicStartTimer();
    EXPECT_EQ(4125u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(0u, pitWaits);
}

TEST(LapicTimer, UsesActualElapsedTimeWhenPitCompletionIsObservedLate)
{
    lapicTestReset();
    calibratedTicks = 226875;
    calibrationTimes = {1000, 1100, 55001000, 55001100};
    lapicStartTimer();
    EXPECT_EQ(4125u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    lapicConfigureTimer(true, 4);
    EXPECT_EQ(16500u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(1u, pitWaits);
}

TEST(LapicTimer, FallsBackWhenAdvertisedRateAndReferenceIntervalAreUnusable)
{
    lapicTestReset();
    apicBusHz = UINT64_MAX;
    calibrationTimes = {50000, 50000, 2000, 2000};
    lapicStartTimer();
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(3u, pitWaits);
}

TEST(LapicTimer, RetriesAReversedReferenceBeforeUsingNominalPitTime)
{
    lapicTestReset();
    calibrationTimes = {50000, 50000, 2000, 2000, 1000000, 1000000, 21000000, 21000000};
    lapicStartTimer();
    EXPECT_EQ(5000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(2u, pitWaits);
}

TEST(LapicTimer, RetriesAnMmioReadSpanningA100msDeschedule)
{
    lapicTestReset();
    calibrationTimes = {1000, 100001000, 110001000, 110001100,
                        200000000, 200000100, 220000000, 220000100};
    pitTickSamples = {100000, 200000};
    lapicStartTimer();
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(2u, pitWaits);
}

TEST(LapicTimer, RetriesAnExhaustedCounterBeforeConsideringNominalPitTime)
{
    lapicTestReset();
    pitTickSamples = {UINT32_MAX, 50000};
    calibrationTimes = {0, 100, 10000000, 10000100, 20000000, 20000100, 30000000, 30000100};
    lapicStartTimer();
    EXPECT_EQ(5000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(2u, pitWaits);
}

TEST(LapicTimer, RejectsRepeatedExhaustionButPreservesFinitePitFallback)
{
    lapicTestReset();
    calibratedTicks = UINT32_MAX;
    EXPECT_EQ(0u, lapicMeasureFrequency());
    EXPECT_EQ(3u, pitWaits);

    lapicTestReset();
    for(uint64_t i = 0; i < 3; ++i)
    {
        const uint64_t base = i * 200000000;
        calibrationTimes.insert(calibrationTimes.end(),
                                {base, base + 100, base + 10000000, base + 110000000});
    }
    EXPECT_EQ(10000000u, lapicMeasureFrequency());
    EXPECT_EQ(3u, pitWaits);
}

TEST(LapicTimer, KeepsBestBoundedSampleIfAllMmioReadsExceedTargetUncertainty)
{
    lapicTestReset();
    calibrationTimes = {0, 0, 10000000, 10600000,
                        20000000, 20210000, 30210000, 30210000,
                        40000000, 40000000, 50000000, 50700000};
    pitTickSamples = {130000, 101050, 115000};
    lapicStartTimer();
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(3u, pitWaits);
}

TEST(LapicTimer, StopsIdleTimerAndRestoresPeriodicModeOnce)
{
    lapicTestReset();
    lapicStartTimer();
    writes.clear();

    lapicConfigureTimer(true, 0);
    EXPECT_EQ(0x20u | APIC_LVT_INT_MASKED, lapicRead(APIC_REGISTER_LVT_TIMER));
    EXPECT_EQ(0u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(g_lapic_timer_mode::stopped, timers[0].mode);
    writes.clear();
    lapicConfigureTimer(true, 0);
    EXPECT_TRUE(writes.empty());

    lapicConfigureTimer(false, 0);
    EXPECT_EQ(0x20u | APIC_LVT_TIMER_MODE_PERIODIC, lapicRead(APIC_REGISTER_LVT_TIMER));
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    writes.clear();
    lapicConfigureTimer(false, 0);
    EXPECT_TRUE(writes.empty());
}

TEST(LapicTimer, ArmsNearestDeadlineAndRearmsAfterExpiration)
{
    lapicTestReset();
    lapicStartTimer();
    lapicConfigureTimer(true, 5);
    EXPECT_EQ(0x20u | APIC_LVT_TIMER_MODE_ONESHOT, lapicRead(APIC_REGISTER_LVT_TIMER));
    EXPECT_EQ(50000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));

    lapicConfigureTimer(true, 2);
    EXPECT_EQ(20000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    registers[0][APIC_REGISTER_TIMER_CURRCNT / 16] = 0;
    writes.clear();
    lapicConfigureTimer(true, 2);
    ASSERT_FALSE(writes.empty());
    EXPECT_EQ(APIC_REGISTER_TIMER_INITCNT, writes.back().reg);
    EXPECT_EQ(20000u, writes.back().value);

    lapicConfigureTimer(false, 0);
    EXPECT_EQ(0x20u | APIC_LVT_TIMER_MODE_PERIODIC, lapicRead(APIC_REGISTER_LVT_TIMER));
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    lapicConfigureTimer(true, 2);
    lapicConfigureTimer(true, 0);
    EXPECT_EQ(0u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(0x20u | APIC_LVT_INT_MASKED, lapicRead(APIC_REGISTER_LVT_TIMER));
}

TEST(LapicTimer, SaturatesOneShotCountsWithoutMultiplicationOverflow)
{
    lapicTestReset();
    calibratedTicks = 10;
    lapicStartTimer();
    lapicConfigureTimer(true, 1);
    EXPECT_EQ(1u, lapicRead(APIC_REGISTER_TIMER_INITCNT));

    apicBusHz = uint64_t(UINT32_MAX) * 1600; // Ten milliseconds fill the downcounter.
    lapicStartTimer();
    lapicConfigureTimer(true, 10);
    EXPECT_EQ(UINT32_MAX, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    lapicConfigureTimer(true, 11);
    EXPECT_EQ(UINT32_MAX, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    lapicConfigureTimer(true, UINT64_MAX);
    EXPECT_EQ(UINT32_MAX, lapicRead(APIC_REGISTER_TIMER_INITCNT));
}

TEST(LapicTimer, RetainsEachProcessorsCalibrationAndMode)
{
    lapicTestReset();
    lapicStartTimer();
    lapicConfigureTimer(true, 0);

    currentProcessor = 255;
    calibratedTicks = 250000;
    lapicStartTimer();
    lapicConfigureTimer(true, 3);
    EXPECT_EQ(75000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(g_lapic_timer_mode::stopped, timers[0].mode);

    currentProcessor = 0;
    lapicConfigureTimer(false, 0);
    EXPECT_EQ(10000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
    EXPECT_EQ(75000u, registers[255][APIC_REGISTER_TIMER_INITCNT / 16]);

    currentProcessor = 255;
    lapicConfigureTimer(false, 0);
    EXPECT_EQ(25000u, lapicRead(APIC_REGISTER_TIMER_INITCNT));
}

TEST(LapicTimer, RejectsUncalibratedOrOutOfRangeProcessors)
{
    lapicTestReset();
    EXPECT_DEATH(lapicConfigureTimer(true, 1), "");
    calibratedTicks = 0;
    EXPECT_DEATH(lapicStartTimer(), "");
    currentProcessor = 256;
    EXPECT_DEATH(lapicStartTimer(), "");
    EXPECT_DEATH(lapicConfigureTimer(false, 0), "");
}

TEST(LapicIpi, KeepsDestinationAndCommandWritesAtomicAgainstLocalInterrupts)
{
    lapicTestReset();
    interruptFlag = true;
    lapicSendIpi(23, 0xf1);
    EXPECT_TRUE(interruptFlag);
    EXPECT_EQ(2u, icrWaits);
    ASSERT_EQ(2u, writes.size());
    EXPECT_EQ(APIC_REGISTER_INT_COMMAND_HIGH, writes[0].reg);
    EXPECT_EQ(23u << 24, writes[0].value);
    EXPECT_EQ(APIC_REGISTER_INT_COMMAND_LOW, writes[1].reg);
    EXPECT_EQ(0xf1u | APIC_ICR_LEVEL_ASSERT, writes[1].value);

    interruptFlag = false;
    lapicSendIpi(7, 0xf2);
    EXPECT_FALSE(interruptFlag);
    EXPECT_EQ(4u, icrWaits);
}

TEST(LapicIpi, BroadcastsFixedVectorToOtherProcessorsWithOneCommandWrite)
{
    lapicTestReset();
    interruptFlag = true;
    lapicSendIpiAllOthers(0xf0);
    EXPECT_TRUE(interruptFlag);
    EXPECT_EQ(2u, icrWaits);
    ASSERT_EQ(1u, writes.size());
    EXPECT_EQ(APIC_REGISTER_INT_COMMAND_LOW, writes[0].reg);
    EXPECT_EQ(0xf0u | APIC_ICR_LEVEL_ASSERT | APIC_ICR_DEST_SHORTHAND_ALL_NOT_SELF, writes[0].value);

    interruptFlag = false;
    lapicSendIpiAllOthers(0xf0);
    EXPECT_FALSE(interruptFlag);
    EXPECT_EQ(4u, icrWaits);
}

TEST(LapicEoi, KeepsMmioUnlessTheHvInterfaceAdvertisesApicMsrs)
{
    for(unsigned stage = 0; stage < 4; ++stage)
    {
        lapicTestReset();
        if(stage >= 1) cpuidLeaves[1][2] = 1u << 31;
        if(stage >= 2) cpuidLeaves[0x40000000][0] = 0x40000003;
        if(stage >= 3) cpuidLeaves[0x40000001][0] = 0x31237648;
        // Even a recognizable interface must grant access to the APIC MSRs.
        lapicDetectHypervisorEoi();
        EXPECT_FALSE(hypervisorEoi);
        EXPECT_EQ(stage + 1, cpuidQueries.size());
        lapicSendEndOfInterrupt();
        ASSERT_EQ(1u, writes.size());
        EXPECT_EQ(APIC_REGISTER_EOI, writes[0].reg);
        EXPECT_EQ(0u, writes[0].value);
        EXPECT_TRUE(msrWrites.empty());
    }
}

TEST(LapicEoi, UsesAdvertisedMsrWithoutDependingOnTheVendorName)
{
    lapicTestReset();
    cpuidLeaves[1][2] = 1u << 31;
    cpuidLeaves[0x40000000] = {0x40000006, 0x786f4256, 0x786f4256, 0x786f4256};
    cpuidLeaves[0x40000001][0] = 0x31237648;
    cpuidLeaves[0x40000003][0] = 1u << 4;
    lapicDetectHypervisorEoi();
    ASSERT_TRUE(hypervisorEoi);
    lapicSendEndOfInterrupt();
    EXPECT_TRUE(writes.empty());
    ASSERT_EQ(1u, msrWrites.size());
    EXPECT_EQ((std::array<uint32_t, 3>{0x40000070, 0, 0}), msrWrites[0]);
}
}
