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
#include <functional>
#include <stdexcept>
#define GHOST_API_STDINT
#include <ghost/tasks/types.h>
#include <ghost/system/irq.h>

namespace
{
#include "production/acpi_irq_layouts.inc"
#include "production/ioapic_flags.inc"

static bool interruptFlag;
struct g_interrupt_guard
{
	bool prior = interruptFlag;
	g_interrupt_guard() { interruptFlag = false; }
	~g_interrupt_guard() { interruptFlag = prior; }
};
struct g_mutex { unsigned depth = 0; };
struct g_mutex_guard
{
	g_mutex* mutex;
	g_interrupt_guard interrupts;
	explicit g_mutex_guard(g_mutex* lock): mutex(lock) { ++mutex->depth; }
	~g_mutex_guard() { --mutex->depth; }
};
void mutexInitializeGlobal(g_mutex* mutex, const char*) { mutex->depth = 0; }
struct g_process { g_pid id; } service{17}, driver{40}, other{70};
struct g_assignment { uint32_t processor; } local{0}, remote{1};
struct g_task
{
	g_tid id;
	g_process* process;
	int securityLevel = G_SECURITY_LEVEL_DRIVER;
	bool terminationRequested = false;
	g_assignment* assignment = &local;
	int status = G_TASK_STATUS_RUNNING;
	g_mutex lock{};
};
g_task worker{42, &driver}, manager{40, &driver}, sci{17, &service}, foreign{70, &other};
static g_task* current;
static g_task* preferred;
static unsigned wakes, switches, pins, yields, clockArms, clockDisarms;
static uint32_t core;
static uint64_t armedDeadline;
static g_tid timedTask;
static std::vector<char> events;
static std::function<void()> duringYield;
struct g_clock_local { uint64_t time; } localClock;

static constexpr uint32_t fixtureRouteCount = 32;
struct g_ioapic { uint32_t globalSystemInterruptBase, redirectEntryCount; g_ioapic* next; };
g_ioapic highIo{16, 16, nullptr}, lowIo{0, 16, &highIo};
g_ioapic* ioapicList = &lowIo;
std::array<uint64_t, fixtureRouteCount> routes;
std::array<bool, 16> picMasked;
struct g_cpu { uint32_t apicId; } bsp{3};
static bool hasApic;
static std::array<uint32_t, 8> lapicIrr, lapicIsr;
static unsigned lapicReads;
uint32_t lapicRead(uint32_t reg)
{
	EXPECT_EQ(core, 0u);
	++lapicReads;
	if(reg >= 0x200 && reg <= 0x270 && !(reg & 15)) return lapicIrr[(reg - 0x200) / 0x10];
	if(reg >= 0x100 && reg <= 0x170 && !(reg & 15)) return lapicIsr[(reg - 0x100) / 0x10];
	ADD_FAILURE() << "unexpected LAPIC register " << std::hex << reg;
	return 0;
}
static uint16_t elcr;
uint8_t ioPortReadByte(uint16_t port)
{
	EXPECT_TRUE(port == 0x4d0 || port == 0x4d1);
	return port == 0x4d0 ? uint8_t(elcr) : uint8_t(elcr >> 8);
}
bool ioapicAreAvailable() { return hasApic; }
g_ioapic* ioapicGetResponsibleFor(uint32_t source) { return source >= fixtureRouteCount ? nullptr : source < 16 ? &lowIo : &highIo; }
uint64_t ioapicGetRedirectionEntry(g_ioapic* io, uint32_t index) { return routes.at(io->globalSystemInterruptBase + index); }
void ioapicSetRedirectionEntry(g_ioapic* io, uint32_t index, uint64_t entry) { routes.at(io->globalSystemInterruptBase + index) = entry; }
void ioapicMaskIrq(uint32_t gsi) { routes.at(gsi) |= IOAPIC_REDTBL_INTMASK_MASKED; events.push_back('M'); }
void ioapicUnmaskIrq(uint32_t gsi) { routes.at(gsi) &= ~uint64_t(IOAPIC_REDTBL_INTMASK_MASKED); events.push_back('A'); }
bool picIsMasked(uint8_t irq) { return picMasked.at(irq); }
void picMaskIrq(uint8_t irq) { picMasked.at(irq) = true; events.push_back('M'); }
void picUnmaskIrq(uint8_t irq) { picMasked.at(irq) = false; events.push_back('A'); }
g_cpu* processorGetBsp() { return &bsp; }
uint32_t processorGetCurrentId() { return core; }
g_tid acpiDriverGetService() { return service.id; }
void interruptsSendEndOfInterrupt(uint8_t irqOffset)
{
	events.push_back('E');
	const uint8_t vector = 0x20 + irqOffset;
	lapicIsr[vector / 32] &= ~(1u << (vector % 32));
	for(auto& entry: routes)
		if((entry & 0xff) == vector)
			entry &= ~uint64_t(IOAPIC_REDTBL_REMOTEIRR_ACCEPTING);
}
void taskingHandleTerminationRequest() {}
void taskingYield()
{
	EXPECT_FALSE(interruptFlag);
	if(++yields > 32 || !duringYield)
		throw std::runtime_error("unexpected yield from IRQ wait");
	duringYield();
}
g_task* taskingAcquireTask(g_tid id)
{
	for(auto target: {&manager, &worker, &sci, &foreign})
		if(target->id == id && target->status != G_TASK_STATUS_DEAD)
		{
			++pins;
			return target;
		}
	return nullptr;
}
void taskingReleaseTask(g_task* task) { if(task) { EXPECT_GT(pins, 0u); --pins; } }
void taskingWake(g_task* task) { task->status = G_TASK_STATUS_RUNNING; ++wakes; events.push_back('W'); }
void taskingPrepareWait(g_task* task, const char*) { EXPECT_FALSE(interruptFlag); task->status = G_TASK_STATUS_WAITING; }
void taskingSetCurrent(g_task* task) { current = task; ++switches; }
void schedulerPreferTask(g_task* task) { preferred = task; }
g_clock_local* clockGetLocal() { return &localClock; }
uint64_t clockGetMilliseconds() { return clockGetLocal()->time; }
void clockWaitForTime(g_tid task, uint64_t deadline)
{
	EXPECT_FALSE(interruptFlag);
	EXPECT_EQ(timedTask, G_TID_NONE);
	timedTask = task;
	armedDeadline = deadline;
	++clockArms;
}
void clockUnwaitForTime(g_task* task)
{
	EXPECT_FALSE(interruptFlag);
	EXPECT_EQ(timedTask, task->id);
	timedTask = G_TID_NONE;
	++clockDisarms;
}
#include "production/tasking_wait_if.inc"
#include "production/irq_backend_types.inc"
#include "production/irq_backend.inc"

class Irq : public testing::Test
{
protected:
	void SetUp() override
	{
		for(auto& slot: irqBindings) slot = {};
		irqPicSlaveBindings = irqRetiringCount = lapicReads = 0;
		lapicIrr.fill(0); lapicIsr.fill(0);
		hasApic = true;
		for(uint32_t i = 0; i < routes.size(); ++i)
			routes[i] = IOAPIC_REDTBL_INTMASK_MASKED | (uint64_t(i + 3) << 56) | 0x46;
		picMasked.fill(true);
		elcr = 1u << 9;
		core = wakes = switches = pins = yields = clockArms = clockDisarms = 0;
		localClock.time = 100;
		armedDeadline = 0;
		timedTask = G_TID_NONE;
		worker = {42, &driver}; manager = {40, &driver}; sci = {17, &service}; foreign = {70, &other};
		current = &manager; preferred = nullptr;
		interruptFlag = true; events.clear(); duringYield = {};
		irqInitialize();
		acpiIrqConfigure(nullptr, nullptr);
	}
	void TearDown() override { EXPECT_EQ(pins, 0u); EXPECT_EQ(timedTask, G_TID_NONE); }
	g_irq_handle install(g_task* task, g_irq_source source, uint32_t line)
	{
		g_irq_handle handle = 0;
		EXPECT_EQ(irqInstall(task, source, line, &handle), G_IRQ_SUCCESSFUL);
		EXPECT_NE(handle, G_IRQ_HANDLE_NONE);
		return handle;
	}
};

TEST_F(Irq, LevelMasksBeforeEoiAndWakeAndRetainsUnreadEventsWhileCompleting)
{
	auto handle = install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
	EXPECT_EQ(routes[20] & 0xff, 0x34u);
	EXPECT_EQ(routes[20] >> 56, bsp.apicId);
	EXPECT_TRUE(routes[20] & IOAPIC_REDTBL_TRIGGERMOD_LEVEL);
	EXPECT_TRUE(routes[20] & IOAPIC_REDTBL_INTPOL_LOW_ACTIVE);
	ASSERT_TRUE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	duringYield = [&] {
		events.clear(); irqHandle(&manager, 20);
		EXPECT_EQ(events, (std::vector<char>{'M', 'E', 'W'}));
		ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
		EXPECT_TRUE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);
	};
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	EXPECT_EQ(wakes, 1u); EXPECT_EQ(switches, 0u);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	EXPECT_FALSE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);

	irqHandle(&manager, 20);
	irqHandle(&manager, 20);
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	EXPECT_EQ(yields, 1u);
	EXPECT_TRUE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);
}

TEST_F(Irq, TimeoutRetriesSpuriousWakesAgainstOneDeadlineRestoresInterruptFlagAndKeepsEdgeArmed)
{
	auto handle = install(&worker, G_IRQ_SOURCE_ISA, 1);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	duringYield = [&] {
		EXPECT_EQ(timedTask, worker.id);
		EXPECT_EQ(armedDeadline, 150u);
		localClock.time = yields == 1 ? 112 : 150;
		taskingWake(&worker);
	};
	EXPECT_EQ(irqWait(&worker, handle, 50), G_IRQ_TIMED_OUT);
	EXPECT_TRUE(interruptFlag);
	EXPECT_EQ(clockArms, 2u); EXPECT_EQ(clockDisarms, 2u);
	EXPECT_FALSE(routes[1] & IOAPIC_REDTBL_INTMASK_MASKED);

	interruptFlag = false;
	duringYield = [&] {
		localClock.time = armedDeadline;
		irqHandle(&manager, 1);
	};
	EXPECT_EQ(irqWait(&worker, handle, 50), G_IRQ_DELIVERED);
	EXPECT_FALSE(interruptFlag);
	EXPECT_EQ(clockArms, 3u); EXPECT_EQ(clockDisarms, 3u);
	irqHandle(&manager, 1);
	events.clear();
	EXPECT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	EXPECT_TRUE(events.empty());
	EXPECT_EQ(irqWait(&worker, handle, 50), G_IRQ_DELIVERED);
	EXPECT_EQ(clockArms, 3u);
}

TEST_F(Irq, ReleaseWakesWaiterRestoresPreviouslyMaskedRouteAndOldHandleCannotTouchNewBinding)
{
	auto previous = routes[20];
	auto handle = install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
	duringYield = [&] {
		ASSERT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
		EXPECT_EQ(routes[20], previous);
	};
	EXPECT_EQ(irqWait(&worker, handle, 100), G_IRQ_RELEASED);
	EXPECT_EQ(wakes, 1u); EXPECT_EQ(clockArms, clockDisarms);
	auto replacement = install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
	EXPECT_NE(handle, replacement);
	EXPECT_EQ(irqWait(&worker, handle, 1), G_IRQ_RELEASED);
	EXPECT_EQ(irqComplete(&manager, handle), G_IRQ_RELEASED);
	EXPECT_EQ(irqRelease(&manager, handle), G_IRQ_RELEASED);
	ASSERT_EQ(irqComplete(&manager, replacement), G_IRQ_SUCCESSFUL);
	irqHandle(&manager, 20);
	irqTaskStopped(&worker);
	EXPECT_EQ(routes[20], previous);
	EXPECT_EQ(irqWait(&worker, replacement, 1), G_IRQ_RELEASED);
}

TEST_F(Irq, RejectsAliasedRoutesInFlightFirmwareReservationsAndOrdinaryProcesses)
{
	g_irq_handle result = UINT64_MAX;
	for(uint32_t line: {0u, 96u, 255u, UINT32_MAX})
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, line, &result), G_IRQ_INVALID_ARGUMENT);
	for(uint32_t line: {1u, 8u, 9u, 12u, 15u})
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, line, &result), G_IRQ_NOT_SUPPORTED);
	for(auto flags: {IOAPIC_REDTBL_DELIVS_SEND_PENDING, IOAPIC_REDTBL_REMOTEIRR_ACCEPTING})
	{
		routes[11] |= flags;
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 11, &result), G_IRQ_BUSY);
		routes[11] &= ~uint64_t(flags);
	}
	routes[11] &= ~uint64_t(IOAPIC_REDTBL_INTMASK_MASKED);
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 11, &result), G_IRQ_BUSY);
	routes[11] |= IOAPIC_REDTBL_INTMASK_MASKED;
	routes[4] = 0x2b;
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 11, &result), G_IRQ_BUSY);
	routes[4] |= IOAPIC_REDTBL_INTMASK_MASKED;
	for(auto flags: {IOAPIC_REDTBL_DELIVS_SEND_PENDING, IOAPIC_REDTBL_REMOTEIRR_ACCEPTING})
	{
		routes[4] |= flags;
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 11, &result), G_IRQ_BUSY);
		routes[4] &= ~uint64_t(flags);
	}
	routes[5] = 0x25;
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ISA, 5, &result), G_IRQ_BUSY);
	EXPECT_EQ(routes[5], 0x25u);
	routes[20] = 0x34;
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 20, &result), G_IRQ_BUSY);
	EXPECT_EQ(routes[20], 0x34u);
	routes[20] |= IOAPIC_REDTBL_INTMASK_MASKED;
	foreign.securityLevel = G_SECURITY_LEVEL_APPLICATION;
	EXPECT_EQ(irqInstall(&foreign, G_IRQ_SOURCE_PCI_INTX, 11, &result), G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ACPI, 9, &result), G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(result, G_IRQ_HANDLE_NONE);

	auto sciHandle = install(&sci, G_IRQ_SOURCE_ACPI, 20);
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 20, &result), G_IRQ_BUSY);
	EXPECT_EQ(irqWait(&worker, sciHandle, 0), G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(irqComplete(&worker, sciHandle), G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(irqRelease(&worker, sciHandle), G_IRQ_NOT_PERMITTED);
	ASSERT_EQ(irqRelease(&sci, sciHandle), G_IRQ_SUCCESSFUL);
	auto pci = install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
	EXPECT_EQ(irqWait(&manager, pci, 1), G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(yields, 0u);
	EXPECT_EQ(irqRelease(&worker, pci), G_IRQ_SUCCESSFUL);
}

TEST_F(Irq, EachLevelNotificationIsConsumedOnce)
{
	for(auto source: {G_IRQ_SOURCE_ACPI, G_IRQ_SOURCE_PCI_INTX})
	{
		auto task = source == G_IRQ_SOURCE_ACPI ? &sci : &worker;
		uint32_t line = source == G_IRQ_SOURCE_ACPI ? 9 : 20;
		auto handle = install(task, source, line);
		ASSERT_EQ(irqComplete(task, handle), G_IRQ_SUCCESSFUL);
		irqHandle(nullptr, line);
		EXPECT_EQ(irqWait(task, handle, 0), G_IRQ_DELIVERED);
		duringYield = [&] { localClock.time = armedDeadline; taskingWake(task); };
		EXPECT_EQ(irqWait(task, handle, 10), G_IRQ_TIMED_OUT);
		EXPECT_NE(routes[line] & IOAPIC_REDTBL_INTMASK_MASKED, 0u);
		ASSERT_EQ(irqComplete(task, handle), G_IRQ_SUCCESSFUL);
		EXPECT_EQ(routes[line] & IOAPIC_REDTBL_INTMASK_MASKED, 0u);
		EXPECT_EQ(irqRelease(task, handle), G_IRQ_SUCCESSFUL);
	}
}

TEST_F(Irq, IsaOverrideExclusivelyOwnsItsGsiAndVectorThroughRetirement)
{
	constexpr size_t offset = sizeof(g_madt_header);
	std::array<uint8_t, offset + sizeof(g_madt_interrupt_src_override_entry)> bytes{};
	auto madt = reinterpret_cast<g_madt_header*>(bytes.data());
	madt->header.length = bytes.size();
	auto override = reinterpret_cast<g_madt_interrupt_src_override_entry*>(bytes.data() + offset);
	override->header = {2, sizeof(*override)};
	override->irqSource = 5; override->globalSystemInterrupt = 20; override->flags = 7;
	acpiIrqConfigure(nullptr, &madt->header);
	auto original = routes[20];
	auto handle = install(&worker, G_IRQ_SOURCE_ISA, 5);
	EXPECT_TRUE(routes[20] & IOAPIC_REDTBL_INTPOL_LOW_ACTIVE);
	EXPECT_FALSE(routes[20] & IOAPIC_REDTBL_TRIGGERMOD_LEVEL);
	EXPECT_EQ(routes[20] & 0xff, 0x25u);
	g_irq_handle other = 0;
	EXPECT_EQ(irqInstall(&manager, G_IRQ_SOURCE_PCI_INTX, 20, &other), G_IRQ_BUSY);
	EXPECT_EQ(irqInstall(&manager, G_IRQ_SOURCE_PCI_INTX, 5, &other), G_IRQ_BUSY);
	EXPECT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	irqHandle(&manager, 5);
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	core = 1;
	ASSERT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
	EXPECT_EQ(irqInstall(&manager, G_IRQ_SOURCE_PCI_INTX, 20, &other), G_IRQ_BUSY);
	EXPECT_EQ(irqInstall(&manager, G_IRQ_SOURCE_PCI_INTX, 5, &other), G_IRQ_BUSY);
	core = 0;
	irqReapRetired();
	EXPECT_EQ(routes[20], original);
	for(uint16_t flags: {2u, 8u})
	{
		override->flags = flags;
		acpiIrqConfigure(nullptr, &madt->header);
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ISA, 5, &other), G_IRQ_NOT_SUPPORTED);
	}
	override->flags = 15;
	acpiIrqConfigure(nullptr, &madt->header);
	auto level = install(&worker, G_IRQ_SOURCE_ISA, 5);
	EXPECT_NE(routes[20] & IOAPIC_REDTBL_TRIGGERMOD_LEVEL, 0u);
	EXPECT_NE(routes[20] & IOAPIC_REDTBL_INTPOL_LOW_ACTIVE, 0u);
	ASSERT_EQ(irqComplete(&manager, level), G_IRQ_SUCCESSFUL);
	irqHandle(&manager, 5);
	ASSERT_TRUE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);
	EXPECT_EQ(irqWait(&worker, level, 0), G_IRQ_DELIVERED);
	EXPECT_EQ(irqComplete(&manager, level), G_IRQ_SUCCESSFUL);
	EXPECT_FALSE(routes[20] & IOAPIC_REDTBL_INTMASK_MASKED);
}

TEST_F(Irq, ReservesFadtSciAndItsOverrideAndMadtNmi)
{
	std::array<uint8_t, 48> fadtBytes{};
	auto fadt = reinterpret_cast<g_acpi_table_header*>(fadtBytes.data());
	fadt->length = fadtBytes.size(); fadtBytes[46] = 10;
	constexpr size_t offset = sizeof(g_madt_header);
	constexpr size_t nmiOffset = offset + sizeof(g_madt_interrupt_src_override_entry);
	std::array<uint8_t, nmiOffset + 8> bytes{};
	auto madt = reinterpret_cast<g_madt_header*>(bytes.data());
	madt->header.length = bytes.size();
	auto override = reinterpret_cast<g_madt_interrupt_src_override_entry*>(bytes.data() + offset);
	override->header = {2, sizeof(*override)}; override->irqSource = 10; override->globalSystemInterrupt = 18;
	bytes[nmiOffset] = 3; bytes[nmiOffset + 1] = 8; bytes[nmiOffset + 4] = 19;
	acpiIrqConfigure(fadt, &madt->header);
	g_irq_handle handle;
	for(auto gsi: {10, 18, 19})
		EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, gsi, &handle), G_IRQ_NOT_SUPPORTED);
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 19, &handle), G_IRQ_NOT_SUPPORTED);
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ISA, 10, &handle), G_IRQ_NOT_SUPPORTED);
	auto first = routes[10];
	for(uint16_t polarity: {1u, 3u})
	{
		override->flags = uint16_t(12 | polarity);
		acpiIrqConfigure(fadt, &madt->header);
		ASSERT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 10, &handle), G_IRQ_SUCCESSFUL);
		EXPECT_EQ(routes[10], first);
		EXPECT_EQ(routes[18] & 0xff, 0x2au);
		EXPECT_NE(routes[18] & IOAPIC_REDTBL_TRIGGERMOD_LEVEL, 0u);
		EXPECT_EQ(bool(routes[18] & IOAPIC_REDTBL_INTPOL_LOW_ACTIVE), polarity == 3);
		EXPECT_EQ(irqRelease(&sci, handle), G_IRQ_SUCCESSFUL);
	}
	override->flags = 4;
	acpiIrqConfigure(fadt, &madt->header);
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 10, &handle), G_IRQ_NOT_SUPPORTED);
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 17, &handle), G_IRQ_SUCCESSFUL);
	EXPECT_EQ(routes[17] & 0xff, 0x31u);
}

TEST_F(Irq, PicRestoresSlaveAndCascadeMasksAcrossOverlappingIsaAndSciBindings)
{
	hasApic = false;
	g_irq_handle out;
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_PCI_INTX, 11, &out), G_IRQ_NOT_SUPPORTED);
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 2, &out), G_IRQ_NOT_SUPPORTED);
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ISA, 2, &out), G_IRQ_INVALID_ARGUMENT);
	EXPECT_TRUE(picMasked[2]); EXPECT_EQ(irqBindings[2].handle, G_IRQ_HANDLE_NONE);
	auto mouse = install(&worker, G_IRQ_SOURCE_ISA, 12);
	auto sciHandle = install(&sci, G_IRQ_SOURCE_ACPI, 9);
	EXPECT_FALSE(picMasked[2]); EXPECT_TRUE(picMasked[9]); EXPECT_TRUE(picMasked[12]);
	EXPECT_EQ(irqComplete(&worker, mouse), G_IRQ_SUCCESSFUL);
	EXPECT_FALSE(picMasked[12]);
	EXPECT_EQ(irqRelease(&worker, mouse), G_IRQ_SUCCESSFUL);
	EXPECT_TRUE(picMasked[12]); EXPECT_FALSE(picMasked[2]);
	EXPECT_EQ(irqRelease(&sci, sciHandle), G_IRQ_SUCCESSFUL);
	EXPECT_TRUE(picMasked[9]); EXPECT_TRUE(picMasked[2]);
}

TEST_F(Irq, PicValidatesFirmwareTriggerAndRetainsEdgeSupportWithNoElcr)
{
	hasApic = false;
	g_irq_handle handle;
	elcr = 0;
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 9, &handle), G_IRQ_NOT_SUPPORTED);
	EXPECT_TRUE(picMasked[2]); EXPECT_TRUE(picMasked[9]);
	elcr = 1u << 12;
	EXPECT_EQ(irqInstall(&worker, G_IRQ_SOURCE_ISA, 12, &handle), G_IRQ_NOT_SUPPORTED);
	elcr = UINT16_MAX;
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 9, &handle), G_IRQ_NOT_SUPPORTED);
	handle = install(&worker, G_IRQ_SOURCE_ISA, 12);
	EXPECT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
	elcr = (1u << 13) | (1u << 9);
	EXPECT_EQ(irqInstall(&sci, G_IRQ_SOURCE_ACPI, 9, &handle), G_IRQ_NOT_SUPPORTED);
	handle = install(&worker, G_IRQ_SOURCE_ISA, 1);
	EXPECT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
}

TEST_F(Irq, AnUnboundInterruptIsAcknowledgedWithoutWakingAFutureOwner)
{
	irqHandle(&manager, 12);
	EXPECT_EQ(events, (std::vector<char>{'E'}));
	EXPECT_EQ(wakes, 0u);

	auto handle = install(&worker, G_IRQ_SOURCE_ISA, 12);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	duringYield = [&] { localClock.time = armedDeadline; taskingWake(&worker); };
	EXPECT_EQ(irqWait(&worker, handle, 10), G_IRQ_TIMED_OUT);
	irqHandle(&manager, 12);
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
}

TEST_F(Irq, HandoffPrefersInterruptedTaskOnlyForLocalRunnableIsaHandler)
{
	auto handle = install(&worker, G_IRQ_SOURCE_ISA, 1);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	irqHandle(&manager, 1);
	EXPECT_EQ(current, &worker); EXPECT_EQ(preferred, &manager);
	EXPECT_EQ(wakes, 0u); EXPECT_EQ(switches, 1u);
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	current = &manager; preferred = nullptr;
	worker.status = G_TASK_STATUS_WAITING;
	irqHandle(&manager, 1);
	EXPECT_EQ(worker.status, G_TASK_STATUS_WAITING); EXPECT_EQ(current, &manager);
	EXPECT_EQ(preferred, nullptr); EXPECT_EQ(wakes, 0u); EXPECT_EQ(switches, 1u);
	worker.status = G_TASK_STATUS_RUNNING; worker.assignment = &remote;
	irqHandle(&manager, 1);
	EXPECT_EQ(current, &manager); EXPECT_EQ(wakes, 0u); EXPECT_EQ(switches, 1u);
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	duringYield = [&] { irqHandle(&manager, 1); };
	EXPECT_EQ(irqWait(&worker, handle, 0), G_IRQ_DELIVERED);
	EXPECT_EQ(wakes, 1u); EXPECT_EQ(switches, 1u);
	worker.assignment = &local;
	irqHandle(&worker, 1);
	EXPECT_EQ(switches, 1u); EXPECT_EQ(pins, 0u);
}

TEST_F(Irq, HandoffWakesTheBoundLocalTaskAndPrefersTheInterruptedTask)
{
	auto handle = install(&worker, G_IRQ_SOURCE_ISA, 12);
	ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
	duringYield = [&] {
		EXPECT_EQ(worker.status, G_TASK_STATUS_WAITING);
		irqHandle(&manager, 12);
		EXPECT_EQ(current, &worker);
		EXPECT_EQ(preferred, &manager);
	};
	EXPECT_EQ(irqWait(&worker, handle, 50), G_IRQ_DELIVERED);
	EXPECT_EQ(wakes, 1u);
	EXPECT_EQ(switches, 1u);
	EXPECT_EQ(clockArms, clockDisarms);
}

TEST_F(Irq, RetiringEdgeOrLevelDropsLateDeliveryBeforeNextGenerationCanUseItsPinOrVector)
{
	for(auto source: {G_IRQ_SOURCE_ISA, G_IRQ_SOURCE_PCI_INTX})
	{
		const uint8_t line = source == G_IRQ_SOURCE_ISA ? 5 : 20;
		const uint8_t vector = 0x20 + line;
		const uint32_t bit = 1u << (vector % 32);
		const auto original = routes[line];
		auto handle = install(&worker, source, line);
		ASSERT_EQ(irqComplete(&manager, handle), G_IRQ_SUCCESSFUL);
		const auto live = routes[line];
		routes[line] |= IOAPIC_REDTBL_DELIVS_SEND_PENDING;
		core = 1;
		auto reads = lapicReads;
		ASSERT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
		EXPECT_EQ(lapicReads, reads);
		EXPECT_EQ(irqComplete(&manager, handle), G_IRQ_RELEASED);
		EXPECT_TRUE(irqBindings[line].retiring);
		EXPECT_EQ(routes[line] & 0xff, vector);
		EXPECT_NE(routes[line] & IOAPIC_REDTBL_INTMASK_MASKED, 0u);
		g_irq_handle retry;
		EXPECT_EQ(irqInstall(&manager, source, line, &retry), G_IRQ_BUSY);
		EXPECT_EQ(lapicReads, reads);

		core = 0;
		irqReapRetired();
		EXPECT_TRUE(irqBindings[line].retiring);
		routes[line] &= ~uint64_t(IOAPIC_REDTBL_DELIVS_SEND_PENDING);
		if(source == G_IRQ_SOURCE_PCI_INTX)
			routes[line] |= IOAPIC_REDTBL_REMOTEIRR_ACCEPTING;
		lapicIrr[vector / 32] |= bit;
		irqReapRetired();
		EXPECT_EQ(irqInstall(&manager, source, line, &retry), G_IRQ_BUSY);
		lapicIrr[vector / 32] &= ~bit;
		lapicIsr[vector / 32] |= bit;
		irqReapRetired();
		EXPECT_EQ(irqInstall(&manager, source, line, &retry), G_IRQ_BUSY);

		auto beforeWakes = wakes;
		irqHandle(&manager, line);
		EXPECT_EQ(wakes, beforeWakes);
		EXPECT_EQ(routes[line], original);
		EXPECT_EQ(irqRetiringCount, 0u);
		ASSERT_EQ(irqInstall(&manager, source, line, &retry), G_IRQ_SUCCESSFUL);
		EXPECT_NE(retry, handle);
		duringYield = [&] { localClock.time = armedDeadline; taskingWake(&manager); };
		EXPECT_EQ(irqWait(&manager, retry, 5), G_IRQ_TIMED_OUT);
		EXPECT_EQ(irqComplete(&manager, retry), G_IRQ_SUCCESSFUL);
		EXPECT_EQ(routes[line], live);
		irqHandle(&manager, line);
		EXPECT_EQ(irqWait(&manager, retry, 5), G_IRQ_DELIVERED);
		ASSERT_EQ(irqRelease(&manager, retry), G_IRQ_SUCCESSFUL);
	}
}

TEST_F(Irq, BspTimerRestoresQuietRemoteReleaseWithoutAnotherDelivery)
{
	const auto original = routes[20];
	auto handle = install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
	core = 1;
	ASSERT_EQ(irqRelease(&manager, handle), G_IRQ_SUCCESSFUL);
	irqReapRetired();
	EXPECT_TRUE(irqBindings[20].retiring);
	EXPECT_EQ(lapicReads, 0u);
	core = 0;
	irqReapRetired();
	EXPECT_EQ(irqRetiringCount, 0u);
	EXPECT_EQ(routes[20], original);
	EXPECT_EQ(wakes, 0u);
	core = 1;
	install(&worker, G_IRQ_SOURCE_PCI_INTX, 20);
}

TEST_F(Irq, OrphanedPicLevelStaysMaskedEvenIfOriginallyEnabled)
{
	hasApic = false;
	picMasked[9] = false;
	auto orderly = install(&sci, G_IRQ_SOURCE_ACPI, 9);
	ASSERT_EQ(irqRelease(&sci, orderly), G_IRQ_SUCCESSFUL);
	EXPECT_FALSE(picMasked[9]);
	auto abandoned = install(&sci, G_IRQ_SOURCE_ACPI, 9);
	ASSERT_EQ(irqComplete(&sci, abandoned), G_IRQ_SUCCESSFUL);
	irqTaskStopped(&sci);
	EXPECT_TRUE(picMasked[9]);
	EXPECT_EQ(irqComplete(&sci, abandoned), G_IRQ_RELEASED);
}

}
