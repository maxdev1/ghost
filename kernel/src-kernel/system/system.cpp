#include <system/system.hpp>
#include <logger/logger.hpp>
#include <system/cpu.hpp>
#include <system/acpi/acpi.hpp>
#include <system/acpi/MADT.hpp>
#include <system/cpu.hpp>
#include <system/interrupts/pic.hpp>
#include <system/interrupts/lapic.hpp>
#include <system/interrupts/ioapic.hpp>
#include <system/interrupts/ioapic_manager.hpp>
#include <system/interrupts/descriptors/idt.hpp>
#include <system/smp/smp.hpp>
#include <kernel.hpp>

static g_cpu* first = 0;
static uint32_t numCores = 0;

/**
 *
 */
g_cpu* g_system::getCpus() {
	return first;
}

/**
 *
 */
uint32_t g_system::getCpuCount() {
	return numCores;
}

/**
 *
 */
uint32_t g_system::getCurrentCoreId() {
	return g_lapic::getCurrentId();
}

/**
 *
 */
void g_system::initializeBsp(g_physical_address initialPageDirectoryPhysical) {

	// Check if the required CPU features are available
	if (g_cpu::supportsCpuid()) {
		g_log_debug("%! supports CPUID", "cpu");
	} else {
		g_kernel::panic("%! no CPUID support", "cpu");
	}

	// Do some CPU info output
	g_cpu::printInformation();

	// APIC must be available
	if (g_cpu::hasFeature(CPUIDStandardEdxFeature::APIC)) {
		g_log_debug("%! APIC available", "cpu");
	} else {
		g_kernel::panic("%! no APIC available", "cpu");
	}

	// Gather ACPI information
	g_acpi::gatherInformation();
	if (g_acpi::hasEntries()) {
		g_log_debug("%! is available", "acpi");

		// Parse the MADT
		g_acpi_entry* cur = g_acpi::getEntryWithSignature("APIC");
		if (cur) {

			// This creates the list of CPU's
			g_madt::parse(cur->header);
		}

	} else {
		g_kernel::panic("%! ACPI info not available", "system");
	}

	// Initialize the interrupt controllers
	if (g_lapic::isPrepared() && g_ioapic_manager::areAvailable() && first) {

		// Initialize the interrupt descriptor table
		g_idt::prepare();
		g_idt::load();

		// Disable PIC properly
		g_pic::remapIrqs();
		g_pic::maskAll();

		// Initialize local APIC
		g_lapic::initialize();

		// Initialize each IO APIC
		g_ioapic* ioapic = g_ioapic_manager::getEntries();
		while (ioapic) {
			ioapic->initialize();
			ioapic = ioapic->getNext();
		}

		// Print available CPUs
		g_log_info("%! %i available core%s", "system", numCores, (numCores > 1 ? "s" : ""));

		// Initialize multiprocessing
		g_smp::initialize(initialPageDirectoryPhysical);

		// Create keyboard and mouse redirection entries
		g_ioapic_manager::createIsaRedirectionEntry(1, 1, 0);
		g_ioapic_manager::createIsaRedirectionEntry(12, 12, 0);

	} else {
		g_kernel::panic("%! pic compatibility mode not implemented. apic/ioapic required!", "system");
		/*
		 PIC::remapIrqs();
		 PIC::unmaskAll();
		 */
	}
}

/**
 * 
 */
void g_system::initializeAp() {

	// Load interrupt descriptor table
	g_idt::load();

	// Initialize local APIC
	g_lapic::initialize();

}

/**
 *
 */
void g_system::createCpu(uint32_t apicId) {

	// Check if ID is in use
	g_cpu* n = first;
	while (n) {
		if (n->apic == apicId) {
			g_log_warn("%! ignoring core with irregular, duplicate id %i", "system", apicId);
			return;
		}
		n = n->next;
	}

	// Create core
	g_cpu* core = new g_cpu();
	core->apic = apicId;
	core->next = first;

	// This function is called by the BSP only, therefore we can do:
	if (apicId == g_lapic::getCurrentId()) {
		core->bsp = true;
	}

	first = core;

	++numCores;
}

/**
 *
 */
g_cpu* g_system::getCpu(uint32_t coreId) {

	g_cpu* n = first;
	while (n) {
		if (n->apic == coreId) {
			return n;
		}
		n = n->next;
	}

	return 0;
}

