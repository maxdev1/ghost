#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>
#define GHOST_API
#define GHOST_API_STDINT
using g_address = uintptr_t;
#include <ghost/system/pci.h>
#include "../../../../libapi/inc/ghost/system/power.h"
using g_tid = int32_t;
using g_message_transaction = int32_t;
constexpr int G_TID_NONE = -1, G_SECURITY_LEVEL_DRIVER = 1;
constexpr int G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1, G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL = 1;
constexpr int G_MESSAGE_SEND_MODE_NON_BLOCKING = 1;
struct g_message_header { g_tid sender; int transaction; size_t length; };
#define G_MESSAGE_CONTENT(message) (reinterpret_cast<uint8_t*>(message) + sizeof(g_message_header))

namespace bridge {
struct g_task { g_tid id; int securityLevel; bool terminationRequested = false; } worker{5, 0}, driver{7, 1};
struct g_clock_local { uint64_t time; } testClock;
static bool replyValid = true, devicePresent = true, spoofFirst;
static unsigned sent;
static g_pci_read_config_request lastRead;
static g_pci_write_config_request lastWrite;
static int command, transaction;
g_clock_local* clockGetLocal() { return &testClock; }
g_task* taskingGetCurrentTask() { return &worker; }
g_task* taskingAcquireTask(int id) { return id == 7 ? &driver : nullptr; }
void taskingReleaseTask(g_task*) {}
g_tid taskingDirectoryGet(const char* name) { EXPECT_STREQ("pcidriver", name); return 7; }
void clockWaitForTime(int, uint64_t until) { testClock.time = until; }
template<class F> void taskingWait(g_task*, const char*, F setup) { setup(); }
int messageQueueNextTxId() { return ++transaction; }
int messageQueueSend(int sender, int target, void* data, size_t length, int) {
    EXPECT_EQ(5, sender); EXPECT_EQ(7, target); ++sent;
    command = static_cast<g_pci_request_header*>(data)->command;
    if(command == G_PCI_READ_CONFIG) {
        EXPECT_EQ(sizeof(lastRead), length); std::memcpy(&lastRead, data, length);
    } else {
        EXPECT_EQ(G_PCI_WRITE_CONFIG, command); EXPECT_EQ(sizeof(lastWrite), length); std::memcpy(&lastWrite, data, length);
    }
    return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}
int messageQueueReceive(int receiver, g_message_header* header, size_t, int tx) {
    EXPECT_EQ(5, receiver); EXPECT_EQ(transaction, tx);
    if(!replyValid) return 0;
    header->sender = spoofFirst ? 99 : 7; spoofFirst = false;
    if(command == G_PCI_READ_CONFIG) {
        header->length = sizeof(g_pci_read_config_response);
        g_pci_read_config_response reply{devicePresent, 0xabcd};
        std::memcpy(G_MESSAGE_CONTENT(header), &reply, sizeof(reply));
    } else {
        header->length = sizeof(g_pci_write_config_response);
        g_pci_write_config_response reply{devicePresent};
        std::memcpy(G_MESSAGE_CONTENT(header), &reply, sizeof(reply));
    }
    return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
}
void memoryCopy(void* out, const void* in, size_t length) { std::memcpy(out, in, length); }
#include "production/pci_driver.inc"

TEST(PciDriver, UsesExistingServiceProtocolAndAuthenticatesReplies) {
    driver.securityLevel = 1; replyValid = devicePresent = true; spoofFirst = true; testClock.time = sent = 0;
    uint32_t value = 0;
    ASSERT_TRUE(pciDriverRead(0x1234, 0x40, 2, &value));
    EXPECT_EQ(0x1234U, uint32_t(lastRead.deviceAddress)); EXPECT_EQ(0x40, lastRead.offset); EXPECT_EQ(2, lastRead.bytes);
    EXPECT_EQ(0xabcdU, value); EXPECT_EQ(1U, testClock.time);
    ASSERT_TRUE(pciDriverWrite(0x5678, 0x44, 4, 0x11223344));
    EXPECT_EQ(0x5678U, uint32_t(lastWrite.deviceAddress)); EXPECT_EQ(0x44, lastWrite.offset); EXPECT_EQ(4, lastWrite.bytes);
    EXPECT_EQ(0x11223344U, uint32_t(lastWrite.value));
}

TEST(PciDriver, RejectsUnprivilegedServicesAndBoundsMissingReplies) {
    driver.securityLevel = 2; replyValid = devicePresent = true; sent = 0;
    uint32_t value = 0;
    EXPECT_FALSE(pciDriverRead(0, 0, 4, &value)); EXPECT_EQ(0U, sent);
    driver.securityLevel = 1; replyValid = false; testClock.time = 0;
    EXPECT_FALSE(pciDriverRead(0, 0, 4, &value)); EXPECT_GE(testClock.time, 15000U);
    replyValid = true; devicePresent = false;
    EXPECT_FALSE(pciDriverRead(0, 0, 4, &value));
}
}

namespace service {
struct g_pci_device { g_pci_device* next; } first{}, second{};
static g_pci_device* deviceList;
static std::map<g_pci_device*, uint16_t> commands;
static unsigned acknowledgements;
static std::vector<size_t> replyLengths;
int g_send_message_tm(int, void* data, size_t length, int, int mode) {
    EXPECT_EQ(G_MESSAGE_SEND_MODE_NON_BLOCKING, mode);
    if(length) EXPECT_EQ(0, static_cast<uint8_t*>(data)[0]);
    replyLengths.push_back(length); return 1;
}
uint16_t pciConfigReadWord(g_pci_device* device, uint8_t offset) { EXPECT_EQ(4, offset); return commands[device]; }
void pciConfigWriteWord(g_pci_device* device, uint8_t offset, uint16_t value) { EXPECT_EQ(4, offset); commands[device] = value; }
int g_power_acknowledge(uint32_t token, uint8_t ready) { EXPECT_EQ(42U, token); EXPECT_TRUE(ready); ++acknowledgements; return 0; }
#include "production/power_pci_driver.inc"

TEST(PciDriverPower, ServiceQuiescesDmaOnlyForKernelRequestsAndRestoresItOnCancellation) {
    first.next = &second; second.next = nullptr; deviceList = &first;
    commands = {{&first, 7}, {&second, 3}}; acknowledgements = 0;
    g_power_event event{G_POWER_EVENT_MAGIC, 42, G_POWER_SHUTDOWN, G_POWER_EVENT_PREPARE, 55};
    g_message_header header{99, 0, sizeof(event)};
    EXPECT_FALSE(pciDriverHandlePower(&header, reinterpret_cast<uint8_t*>(&event)));
    EXPECT_EQ(7, commands[&first]); EXPECT_EQ(0U, acknowledgements);
    header.sender = G_TID_NONE;
    EXPECT_TRUE(pciDriverHandlePower(&header, reinterpret_cast<uint8_t*>(&event)));
    EXPECT_EQ(3, commands[&first]); EXPECT_EQ(3, commands[&second]); EXPECT_EQ(55, pciPowerCoordinator);
    event.type = G_POWER_EVENT_CANCEL; event.request = 43;
    pciDriverHandlePower(&header, reinterpret_cast<uint8_t*>(&event)); EXPECT_EQ(3, commands[&first]);
    event.request = 42;
    pciDriverHandlePower(&header, reinterpret_cast<uint8_t*>(&event));
    EXPECT_EQ(7, commands[&first]); EXPECT_EQ(3, commands[&second]); EXPECT_EQ(2U, acknowledgements);
    EXPECT_EQ(G_TID_NONE, pciPowerCoordinator);
}
}

namespace service {
TEST(PciDriverPower, QuiescedServiceRepliesWithFailureWithoutStrandingClients) {
    g_message_header header{99, 0, sizeof(g_pci_request_header)};
    replyLengths.clear();
    pciDriverRejectPowerRequest(&header, G_PCI_READ_CONFIG);
    EXPECT_EQ((std::vector<size_t>{sizeof(g_pci_read_config_response)}), replyLengths);
    replyLengths.clear();
    pciDriverRejectPowerRequest(&header, G_PCI_LIST_DEVICES);
    EXPECT_EQ((std::vector<size_t>{sizeof(g_pci_list_devices_count_response), 0}), replyLengths);
}
}

namespace resources {
struct g_pci_device {} device;
static bool present, enabled, successful;
static unsigned updates;
g_pci_device* _pciDriverGetDevice(uint32_t) { return present ? &device : nullptr; }
void pciEnableResourceAccess(g_pci_device*, bool value) { enabled = value; ++updates; }
int g_send_message_t(g_tid target, void* data, size_t size, int transaction) {
    EXPECT_EQ(55, target); EXPECT_EQ(77, transaction);
    EXPECT_EQ(sizeof(g_pci_enable_resource_access_response), size);
    successful = static_cast<g_pci_enable_resource_access_response*>(data)->successful;
    return 1;
}
#include "production/pci_resource_access.inc"

TEST(PciDriverResources, UsesTheRequestedEnableFlagIndependentlyOfTheDeviceAddress) {
    present = true; updates = 0;
    for(auto address : {uint32_t(0), uint32_t(0x100)}) {
        for(bool value : {false, true}) {
            g_pci_enable_resource_access_request request{{G_PCI_ENABLE_RESOURCE_ACCESS}, address, uint8_t(value)};
            pciDriverHandleEnableResourceAccess(55, 77, &request);
            EXPECT_TRUE(successful); EXPECT_EQ(value, enabled);
        }
    }
    EXPECT_EQ(4u, updates);
    g_pci_enable_resource_access_request request{{G_PCI_ENABLE_RESOURCE_ACCESS}, 0, 2};
    pciDriverHandleEnableResourceAccess(55, 77, &request);
    EXPECT_FALSE(successful); EXPECT_EQ(4u, updates);
    present = false; request.enabled = 1;
    pciDriverHandleEnableResourceAccess(55, 77, &request);
    EXPECT_FALSE(successful); EXPECT_EQ(4u, updates);
}
}
