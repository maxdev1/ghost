/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <system/system.hpp>
#include <logger/logger.hpp>
#include <system/acpi/acpi.hpp>
#include <system/acpi/MADT.hpp>
#include <system/interrupts/pic.hpp>
#include <system/interrupts/lapic.hpp>
#include <system/interrupts/ioapic.hpp>
#include <system/interrupts/ioapic_manager.hpp>
#include <system/interrupts/descriptors/idt.hpp>
#include <system/smp/smp.hpp>
#include <kernel.hpp>
#include <system/processor.hpp>
#include <video/pretty_boot.hpp>

static g_processor* processor_list = 0;
static uint32_t processors_available = 0;

/**
 *
 */
g_processor* g_system::getProcessorList() {
	return processor_list;
}

/**
 *
 */
uint32_t g_system::getNumberOfProcessors() {
	return processors_available;
}

/**
 *
 */
uint32_t g_system::currentProcessorId() {
	return g_lapic::read_id();
}

/**
 *
 */
void g_system::initializeBsp(g_physical_address initialPageDirectoryPhysical) {

	// Check if the required CPU features are available
	if (g_processor::supportsCpuid()) {
		g_log_debug("%! supports CPUID", "cpu");
	} else {
		g_kernel::panic("%! no CPUID support", "cpu");
	}

	// Do some CPU info output
	g_processor::printInformation();

	// Enable SSE if available
	checkAndEnableSSE();

	// APIC must be available
	if (g_processor::hasFeature(g_cpuid_standard_edx_feature::APIC)) {
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
	if (g_lapic::isPrepared() && g_ioapic_manager::areAvailable() && processor_list) {

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
		g_log_info("%! %i available core%s", "system", processors_available, (processors_available > 1 ? "s" : ""));
		G_PRETTY_BOOT_STATUS("Initializing physical cores", 60);

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

	// Enable SSE if available
	checkAndEnableSSE();

	// Initialize local APIC
	g_lapic::initialize();

}

/**
 *
 */
void g_system::checkAndEnableSSE() {

	if (g_processor::hasFeature(g_cpuid_standard_edx_feature::SSE)) {
		g_log_info("%! support enabled", "sse");
		g_processor::enableSSE();
	} else {
		g_log_warn("%! no support detected", "sse");
	}
}

/**
 *
 */
void g_system::addProcessor(uint32_t apicId) {

	// Check if ID is in use
	g_processor* n = processor_list;
	while (n) {
		if (n->apic == apicId) {
			g_log_warn("%! ignoring core with irregular, duplicate id %i", "system", apicId);
			return;
		}
		n = n->next;
	}

	// Create core
	g_processor* core = new g_processor();
	core->apic = apicId;
	core->next = processor_list;

	// This function is called by the BSP only, therefore we can do:
	if (apicId == g_lapic::read_id()) {
		core->bsp = true;
	}

	processor_list = core;

	++processors_available;
}

/**
 *
 */
g_processor* g_system::getProcessorById(uint32_t coreId) {

	g_processor* n = processor_list;
	while (n) {
		if (n->apic == coreId) {
			return n;
		}
		n = n->next;
	}

	return 0;
}

