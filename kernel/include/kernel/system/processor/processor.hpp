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

#ifndef __KERNEL_PROCESSOR__
#define __KERNEL_PROCESSOR__

#include "ghost/stdint.h"
#include "kernel/system/interrupts/lapic.hpp"
#include "kernel/system/processor/processor_state.hpp"

/**
 * CPUID.1 feature flags
 */
enum class g_cpuid_standard_edx_feature
{

	FPU = 1 << 0, // Onboard x87 FPU
	VME = 1 << 1, // Virtual 8086 supported
	DE = 1 << 2, // Debugging extensions
	PSE = 1 << 3, // Page size extension
	TSC = 1 << 4, // Time stamp counter
	MSR = 1 << 5, // Model specific registers
	PAE = 1 << 6, // Physical address extension
	MCE = 1 << 7, // Machine check exception
	CX8 = 1 << 8, // CMPXCHG8 instruction
	APIC = 1 << 9, // APIC available
	SEP = 1 << 11, // SYSENTER / SYSEXIT
	MTRR = 1 << 12, // Memory type range registers
	PGE = 1 << 13, // Page global enable bit in CR4
	MCA = 1 << 14, // Machine check architecture
	CMOV = 1 << 15, // Cond. move / FCMOV instructions
	PAT = 1 << 16, // Page attribute table
	PSE36 = 1 << 17, // 36 bit page size extension
	PSN = 1 << 18, // Processor serial number
	CLF = 1 << 19, // CLFLUSH instruction
	DTES = 1 << 21, // Debug store available
	ACPI_THERMAL = 1 << 22, // on-board thermal control MSRs
	MMX = 1 << 23, // MMX instructions
	FXSR = 1 << 24, // FXSAVE, FXRESTOR instructions
	SSE = 1 << 25, // Streaming SIMD Extensions
	SSE2 = 1 << 26, // Streaming SIMD Extensions 2
	SS = 1 << 27, // CPU cache supports self-snoop
	HTT = 1 << 28, // Hyperthreading
	TM1 = 1 << 29, // Thermal monitor auto-limits temperature
	IA64 = 1 << 30, // Processor is IA64 that emulates x86
	PBE = 1 << 31, // Pending break enable wakeup support

};

/**
 * CPUID.1 feature flags
 */
enum class g_cpuid_extended_ecx_feature
{

	SSE3 = 1 << 0,
	PCLMUL = 1 << 1,
	DTES64 = 1 << 2,
	MONITOR = 1 << 3,
	DS_CPL = 1 << 4,
	VMX = 1 << 5,
	SMX = 1 << 6,
	EST = 1 << 7,
	TM2 = 1 << 8,
	SSSE3 = 1 << 9,
	CID = 1 << 10,
	FMA = 1 << 12,
	CX16 = 1 << 13,
	ETPRD = 1 << 14,
	PDCM = 1 << 15,
	DCA = 1 << 18,
	SSE4_1 = 1 << 19,
	SSE4_2 = 1 << 20,
	x2APIC = 1 << 21,
	MOVBE = 1 << 22,
	POPCNT = 1 << 23,
	AES = 1 << 25,
	XSAVE = 1 << 26,
	OSXSAVE = 1 << 27,
	AVX = 1 << 28
};

/**
 * Model specific registers
 */
#define IA32_APIC_BASE_MSR			0x1B
#define IA32_APIC_BASE_MSR_BSP		0x100
#define IA32_APIC_BASE_MSR_ENABLE	0x800

struct g_processor
{
	uint32_t id;
	uint32_t hardwareId;
	uint32_t apicId;
	g_processor* next;
	bool bsp;
};

/**
 * Checks whether the CPU supports the CPUID instruction.
 */
extern "C" bool _checkForCPUID();

/**
 * Checks if the processor supports CPUID.
 */
bool processorSupportsCpuid();

/**
 * Performs a CPUID call and returns values in the out parameters.
 */
void processorCpuid(uint32_t code, uint32_t* outA, uint32_t* outB, uint32_t* outC, uint32_t* outD);

/**
 * Enables SSE on the current processor.
 */
extern "C" void _enableSSE();

/**
 * Initializes the bootstrap processor.
 */
void processorInitializeBsp();

/**
 * Initializes an application processor. Is called for each application processor
 * in the system.
 */
void processorInitializeAp();

/**
 * Adds a processor to the list of processors. This is called when the MADT
 * tables are parsed.
 *
 * Each processor gets a consecutive id assigned. This can be different to
 * the processors hardware id.
 *
 * @param apicId the id of the local APIC
 * @param processorId the processor id provided in the MADT
 */
void processorAdd(uint32_t apicId, uint32_t hardwareId);

/**
 * Returns the list of existing processors.
 */
g_processor* processorGetList();

/**
 * Checks if the list of processors was already loaded.
 */
bool processorListAvailable();

/**
 * Returns the number of available processors.
 */
uint16_t processorGetNumberOfProcessors();

/**
 * Returns the logical id of the current processor.
 */
uint32_t processorGetCurrentId();

/**
 * Creates an id mapping table that contains a mapping from
 * APIC id to processor logical id.
 */
void processorApicIdCreateMappingTable();

/**
 * Checks if the processor supports the given standard EDX feature.
 */
bool processorHasFeature(g_cpuid_standard_edx_feature feature);

/**
 * Checks if the processor supports the given standard ECX feature.
 */
bool processorHasFeature(g_cpuid_extended_ecx_feature feature);

/**
 * Prints information about the processor.
 */
void processorPrintInformation();

/**
 * Enables SSE on the processor.
 */
void processorEnableSSE();

/**
 * Returns the CPU's vendor. "out" must be a pointer to a
 * buffer of at least 12 bytes.
 */
void processorGetVendor(char* out);

/**
 * Reads the model-specific register.
 */
void processorReadMsr(uint32_t msr, uint32_t* lo, uint32_t* hi);

/**
 * Writes the model-specific register.
 */
void processorWriteMsr(uint32_t msr, uint32_t lo, uint32_t hi);

/**
 * Reads the EFLAGS register.
 */
uint32_t processorReadEflags();

#endif
