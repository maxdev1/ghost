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
#include <map>
#include <cstring>
#define GHOST_API
#define GHOST_API_STDINT
using g_address = uintptr_t;
#include <ghost/system/acpi.h>
#include <ghost/system/pci.h>

namespace {
using g_tid = int32_t;
using g_message_transaction = int32_t;
using g_virtual_address = uintptr_t;
using g_physical_address = uintptr_t;
constexpr int G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1, G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL = 1;
constexpr uintptr_t G_PAGE_SIZE = 4096, G_PAGE_TABLE_KERNEL_DEFAULT = 3, G_PAGE_KERNEL_UNCACHED = 19;
#define G_PAGE_ALIGN_DOWN(value) ((value) & ~uintptr_t(4095))
struct g_task { int id = 5; int securityLevel = 0; bool terminationRequested = false; } worker, driver{17, 1};
struct g_message_header { int sender; size_t length; };
#define G_MESSAGE_CONTENT(bytes) ((uint8_t*) (bytes) + sizeof(g_message_header))
struct { uint64_t time; } tick;
static int service = 17;
static bool spoof, badVersion, badCommand, shortReply, mapWorks;
static g_acpi_header lastHeader;
static g_acpi_power_request lastPowerRequest;
static bool badToken;
static g_acpi_power_plan replyPlan;
static std::vector<std::pair<unsigned, uint64_t>> writes;
static std::vector<uintptr_t> unmapped;
static unsigned mapCount;
static int failMapping = -1;
alignas(4096) static uint8_t mappedMemory[4096 * 8];
void* memoryVirtualRangePool;
auto clockGetLocal() { return &tick; }
uint64_t clockGetMilliseconds() { return clockGetLocal()->time; }
g_task* taskingGetCurrentTask() { return &worker; }
struct g_interrupt_guard {};
void taskingPrepareWait(g_task*, const char*) {}
void clockWaitForTime(int, uint64_t until) { tick.time = until; }
void taskingHandleTerminationRequest() {}
void taskingYield() {}
template<class F> void taskingWait(g_task*, const char*, F setup) { setup(); }
uint64_t processorReadTsc() { static uint64_t count; return count += 1000000; }
g_tid taskingDirectoryGet(const char* name) { EXPECT_STREQ(G_ACPI_DRIVER_NAME, name); return service; }
g_task* taskingAcquireTask(int id) { return id == 17 ? &driver : nullptr; }
void taskingReleaseTask(g_task*) {}
int messageQueueNextTxId() { return 1; }
int messageQueueSend(int sender, int target, const void* data, size_t size, int) {
    EXPECT_EQ(17, target);
    memcpy(&lastHeader, data, sizeof(lastHeader));
    if(lastHeader.command == G_ACPI_GET_INFO) {
        EXPECT_EQ(worker.id, sender); EXPECT_EQ(sizeof(g_acpi_get_info_request), size);
    } else {
        EXPECT_EQ(G_TID_NONE, sender); EXPECT_EQ(sizeof(lastPowerRequest), size);
        memcpy(&lastPowerRequest, data, sizeof(lastPowerRequest));
        EXPECT_EQ(worker.id, lastPowerRequest.coordinator);
    }

    return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

int messageQueueReceive(int, g_message_header* header, size_t capacity, int) {
    header->sender = spoof ? 99 : 17; spoof = false;
    g_acpi_response_header responseHeader{G_ACPI_MAGIC, uint16_t(badVersion ? 9 : G_ACPI_VERSION),
                                        g_acpi_command(badCommand ? 0 : lastHeader.command), G_ACPI_SUCCESSFUL};
    if(lastHeader.command == G_ACPI_GET_INFO) {
        EXPECT_GE(capacity, sizeof(g_message_header) + sizeof(g_acpi_get_info_response));
        header->length = sizeof(g_acpi_get_info_response);
        auto response = (g_acpi_get_info_response*) G_MESSAGE_CONTENT(header);
        *response = {responseHeader, {}};
        response->info.namespaceReady = 1;
        response->info.sleep[5] = {1, 5, 5};
    } else if(lastHeader.command == G_ACPI_PREPARE) {
        EXPECT_GE(capacity, sizeof(g_message_header) + sizeof(g_acpi_prepare_response));
        header->length = sizeof(g_acpi_prepare_response);
        auto response = (g_acpi_prepare_response*) G_MESSAGE_CONTENT(header);
        *response = {responseHeader, lastPowerRequest.token + unsigned(badToken), replyPlan};
    } else {
        EXPECT_GE(capacity, sizeof(g_message_header) + sizeof(g_acpi_cancel_response));
        header->length = sizeof(g_acpi_cancel_response);
        auto response = (g_acpi_cancel_response*) G_MESSAGE_CONTENT(header);
        *response = {responseHeader, lastPowerRequest.token + unsigned(badToken)};
    }

    if(shortReply)
        --header->length;

    return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
}

void memoryCopy(void* out, const void* in, size_t n) { memcpy(out, in, n); }
bool memoryPhysicalIsRangeAddressable(uint64_t address, size_t n) { return address && n && address <= UINT64_MAX - n; }
uintptr_t addressRangePoolAllocate(void*, unsigned) { return mapCount < 8 ? reinterpret_cast<uintptr_t>(mappedMemory + mapCount++ * 4096) : 0; }
void addressRangePoolFree(void*, uintptr_t) {}
bool pagingMapPage(uintptr_t, uint64_t, uintptr_t, uintptr_t flags) { EXPECT_EQ(G_PAGE_KERNEL_UNCACHED, flags); return mapWorks && (failMapping < 0 || int(mapCount) != failMapping); }
void pagingUnmapPage(uintptr_t address) { unmapped.push_back(address); }
void ioPortWriteByte(uint16_t port, uint8_t value) { writes.emplace_back(port, value); }
void ioPortWriteShort(uint16_t port, uint16_t value) { writes.emplace_back(port, value); }
void ioPortWriteInt(uint16_t port, uint32_t value) { writes.emplace_back(port, value); }
uint8_t ioPortReadByte(uint16_t) { return 0; }
bool pciDriverWrite(uint32_t, uint8_t, uint8_t, uint32_t) { return true; }
#include "production/acpi_driver.inc"
#include "production/acpi_power_bridge.inc"

class AcpiBridge : public testing::Test {
    void SetUp() override {
        powerAcpiReleasePlan(); service = 17; tick.time = 0; spoof = badVersion = badCommand = shortReply = badToken = false; mapWorks = true;
        mapCount = 0; failMapping = -1; unmapped.clear(); writes.clear(); replyPlan = {}; powerAcpiPrepared = false;
        powerAcpiToken = 0; powerAcpiTicksPerMs = 1;
    }
};

TEST_F(AcpiBridge, AuthenticatesDriverRepliesAndExecutesOnlyPreparedWrites) {
    spoof = true; powerAcpiInitialize();

    EXPECT_TRUE(powerAcpiSupported(G_POWER_SHUTDOWN));

    replyPlan.count = 2; replyPlan.retryStart = 1;
    replyPlan.writes[0] = {0x604, 0x1401, G_ACPI_IO, 2};
    replyPlan.writes[1] = {0x604, 0x3401, G_ACPI_IO, 2};

    ASSERT_TRUE(powerAcpiPrepare(G_POWER_SHUTDOWN));

    powerAcpiPerform(G_POWER_SHUTDOWN);

    EXPECT_EQ((std::vector<std::pair<unsigned, uint64_t>>{{0x604, 0x1401}, {0x604, 0x3401}, {0x604, 0x3401}}), writes);

    powerAcpiCancel(G_POWER_SHUTDOWN);

    EXPECT_EQ(G_ACPI_CANCEL, lastHeader.command); EXPECT_FALSE(powerAcpiPrepared);
}

TEST_F(AcpiBridge, RejectsMalformedPlansAndReleasesPartialMmioMappings) {
    replyPlan.count = 2; replyPlan.writes[0] = {0x1000, 0x1234, G_ACPI_MEMORY, 2};
    replyPlan.writes[1] = {0x10001, 0, G_ACPI_IO, 2};

    EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN)); EXPECT_TRUE(unmapped.empty()); EXPECT_EQ(0, mapCount); EXPECT_EQ(0, powerAcpiPlan.count);

    replyPlan.count = 1; replyPlan.writes[0] = {0x604, 0x10000, G_ACPI_IO, 2};

    EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN));

    replyPlan.writes[0] = {0x604, 0, G_ACPI_IO, 3}; EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN));

    replyPlan.count = G_ACPI_MAX_WRITES + 1; EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN));

    replyPlan.count = 0; EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN));

    EXPECT_TRUE(powerAcpiPrepare(G_POWER_REBOOT));
}

TEST_F(AcpiBridge, ReleasesEarlierMappingsWhenALaterMmioMappingFails) {
    replyPlan.count = 2;
    replyPlan.writes[0] = {0x1000, 0x1234, G_ACPI_MEMORY, 2};
    replyPlan.writes[1] = {0x2000, 0x5678, G_ACPI_MEMORY, 2};
    failMapping = 2;

    EXPECT_FALSE(powerAcpiPrepare(G_POWER_SHUTDOWN));
    EXPECT_EQ(1U, unmapped.size());
    EXPECT_EQ(0, powerAcpiPlan.count);
    for(auto mapping : powerAcpiMappings) EXPECT_EQ(0U, mapping);
}

TEST_F(AcpiBridge, RejectsUnavailableDriversAndProtocolMismatches) {
    badVersion = true; EXPECT_FALSE(powerAcpiPrepare(G_POWER_REBOOT));

    badVersion = false; badToken = true; EXPECT_FALSE(powerAcpiPrepare(G_POWER_REBOOT));

    badToken = false; service = G_TID_NONE; EXPECT_FALSE(powerAcpiPrepare(G_POWER_REBOOT));

    EXPECT_TRUE(writes.empty());
}

TEST_F(AcpiBridge, RejectsWrongCommandsTruncatedPayloadsAndCancellationTokens)
{
    g_acpi_info info{};
    info.namespaceReady = 42;
    badCommand = true;
    EXPECT_FALSE(acpiDriverGetInfo(&info));
    EXPECT_EQ(42, info.namespaceReady);

    badCommand = false;
    shortReply = true;
    EXPECT_FALSE(acpiDriverGetInfo(&info));
    EXPECT_EQ(42, info.namespaceReady);

    g_acpi_power_plan plan{};
    plan.count = 3;
    EXPECT_FALSE(acpiDriverPrepare(G_POWER_SHUTDOWN, 7, &plan));
    EXPECT_EQ(3, plan.count);
    EXPECT_FALSE(acpiDriverCancel(G_POWER_SHUTDOWN, 7));

    shortReply = false;
    badToken = true;
    EXPECT_FALSE(acpiDriverCancel(G_POWER_SHUTDOWN, 7));

    badToken = false;
    EXPECT_TRUE(acpiDriverCancel(G_POWER_SHUTDOWN, 7));
}
}
