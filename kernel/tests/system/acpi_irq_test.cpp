#include "support/fixture.hpp"
#include <functional>
#define GHOST_API_STDINT
#include "../../../../libapi/inc/ghost/system/acpi.h"
namespace {
using g_tid = int32_t;
constexpr int G_TID_NONE = -1;
#include "production/ioapic_flags.inc"
struct g_mutex {} acpiPlatformLock;
struct g_interrupt_guard {};
struct g_mutex_guard { explicit g_mutex_guard(g_mutex*) {} };
void mutexAcquire(g_mutex*) {}
void mutexRelease(g_mutex*) {}
struct g_process { int id; } acpiProcess{17}, foreignProcess{44};
struct g_task { int id; g_process* process; bool terminationRequested = false; } endpoint{33, &acpiProcess}, mainTask{17, &acpiProcess}, foreign{44, &foreignProcess};
struct g_ioapic { uint32_t globalSystemInterruptBase; } io{0};
struct g_cpu { bool bsp; uint32_t apicId; g_cpu* next; } cpu{true, 2, nullptr};
static int acpiPlatformIrqs[96];
static bool acpiPlatformPending[96];
static bool masked;
static uint64_t route;
static unsigned wakes;
static unsigned waits;
static std::function<void()> onWait;
bool ioapicAreAvailable() { return true; }
void ioapicMaskIrq(unsigned) { masked = true; }
void ioapicUnmaskIrq(unsigned) { masked = false; }
void picMaskIrq(unsigned) {}
void picUnmaskIrq(unsigned) {}
g_ioapic* ioapicGetResponsibleFor(unsigned) { return &io; }
g_cpu* processorGetBsp() { return &cpu; }
void ioapicSetRedirectionEntry(g_ioapic*, unsigned index, uint64_t entry) { EXPECT_EQ(9U, index); route = entry; masked = true; }
g_task* taskingAcquireTask(int id) { return id == 33 ? &endpoint : id == 17 ? &mainTask : nullptr; }
void taskingReleaseTask(g_task*) {}
void taskingWake(g_task*) { ++wakes; }
void taskingPrepareWait(g_task*, const char*) {}
g_task* requestsGetHandlerTask(unsigned) { return nullptr; }
g_tid acpiDriverGetService() { return 17; }
void taskingHandleTerminationRequest() {}
void taskingYield() { ++waits; if(onWait) onWait(); }
#include "production/tasking_wait_if.inc"
#include "production/acpi_irq_bridge.inc"

class AcpiIrq : public testing::Test
{
    void SetUp() override
    {
        for(auto& id : acpiPlatformIrqs) id = G_TID_NONE;
        for(auto& pending : acpiPlatformPending) pending = false;
        masked = false; route = 0; wakes = waits = 0; onWait = {};
        endpoint.terminationRequested = false;
    }
};

TEST_F(AcpiIrq, MasksLevelInterruptsUntilDriverRearmsAndRemovesOwnershipSafely) {
    EXPECT_EQ(1U << 13, IOAPIC_REDTBL_INTPOL_LOW_ACTIVE);
    EXPECT_EQ(1U << 15, IOAPIC_REDTBL_TRIGGERMOD_LEVEL);
    EXPECT_EQ(1U << 16, IOAPIC_REDTBL_INTMASK_MASKED);
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    EXPECT_TRUE(masked); EXPECT_NE(0U, route & IOAPIC_REDTBL_TRIGGERMOD_LEVEL);
    EXPECT_NE(0U, route & IOAPIC_REDTBL_INTPOL_LOW_ACTIVE);
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRearmIrq(&endpoint, 9));
    EXPECT_FALSE(masked);
    onWait = [] { EXPECT_TRUE(acpiPlatformHandleIrq(nullptr, 9)); };
    EXPECT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformWait(&endpoint, 9));
    EXPECT_TRUE(masked); EXPECT_EQ(1U, wakes); EXPECT_EQ(1U, waits);
    EXPECT_EQ(G_ACPI_NOT_PERMITTED, acpiPlatformRemoveIrq(&foreign, 9));
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRemoveIrq(&mainTask, 9));
    EXPECT_EQ(G_TID_NONE, acpiPlatformIrqs[9]); EXPECT_EQ(2U, wakes);
    EXPECT_EQ(G_ACPI_NOT_PERMITTED, acpiPlatformWait(&endpoint, 9));
    EXPECT_FALSE(acpiPlatformHandleIrq(nullptr, 9));
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    acpiPlatformTaskStopped(&endpoint); EXPECT_TRUE(masked); EXPECT_EQ(G_TID_NONE, acpiPlatformIrqs[9]);
}

TEST_F(AcpiIrq, HandlesPendingAndRepeatedInterruptsAfterRearming)
{
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRearmIrq(&endpoint, 9));
    ASSERT_TRUE(acpiPlatformHandleIrq(nullptr, 9));
    EXPECT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformWait(&endpoint, 9));
    EXPECT_EQ(0U, waits);
    onWait = [] { EXPECT_TRUE(acpiPlatformHandleIrq(nullptr, 9)); };
    for(unsigned iteration = 1; iteration <= 3; ++iteration)
    {
        ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRearmIrq(&endpoint, 9));
        EXPECT_FALSE(masked);
        EXPECT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformWait(&endpoint, 9));
        EXPECT_TRUE(masked); EXPECT_EQ(iteration, waits);
        EXPECT_EQ(iteration + 1, wakes); EXPECT_EQ(33, acpiPlatformIrqs[9]);
    }
}

TEST_F(AcpiIrq, RetriesSpuriousWakesUntilAnInterruptArrives)
{
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRearmIrq(&endpoint, 9));
    onWait = [] { if(waits >= 3) EXPECT_TRUE(acpiPlatformHandleIrq(nullptr, 9)); };
    EXPECT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformWait(&endpoint, 9));
    EXPECT_EQ(3U, waits); EXPECT_EQ(1U, wakes); EXPECT_TRUE(masked);
}

TEST_F(AcpiIrq, StopsWaitingWhenTheHandlerIsRemoved)
{
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    onWait = [] { EXPECT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRemoveIrq(&mainTask, 9)); };
    EXPECT_EQ(G_ACPI_NOT_PERMITTED, acpiPlatformWait(&endpoint, 9));
    EXPECT_EQ(1U, waits); EXPECT_EQ(G_TID_NONE, acpiPlatformIrqs[9]);
}

TEST_F(AcpiIrq, ReinstallingAnIrqDiscardsThePreviousOwnersPendingEvent)
{
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    ASSERT_TRUE(acpiPlatformHandleIrq(nullptr, 9));
    ASSERT_TRUE(acpiPlatformPending[9]);
    acpiPlatformTaskStopped(&endpoint);
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&mainTask, 9));
    EXPECT_FALSE(acpiPlatformPending[9]);
    EXPECT_EQ(uint64_t(cpu.apicId), route >> 56);
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformRemoveIrq(&mainTask, 9));
    EXPECT_FALSE(acpiPlatformPending[9]);
}

TEST_F(AcpiIrq, RejectsInvalidIrqsAndTerminatingOwnersWithoutWaiting)
{
    EXPECT_EQ(G_ACPI_INVALID_ARGUMENT, acpiPlatformWait(&endpoint, 0));
    EXPECT_EQ(G_ACPI_INVALID_ARGUMENT, acpiPlatformWait(&endpoint, 96));
    ASSERT_EQ(G_ACPI_SUCCESSFUL, acpiPlatformInstallIrq(&endpoint, 9));
    endpoint.terminationRequested = true;
    EXPECT_EQ(G_ACPI_NOT_PERMITTED, acpiPlatformWait(&endpoint, 9));
    EXPECT_EQ(0U, waits);
}
}
