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

#include "kernel/system/processor/processor.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/system.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/memory/gdt.hpp"
#include "shared/panic.hpp"

static g_processor* processors = nullptr;
static uint32_t processorsAvailable = 0;
static uint32_t* apicIdToProcessorMapping = nullptr;

/**
 * @return the current processor structure; only available after all cores have
 *	been initialized and the system was marked ready
 */
g_processor* _processorGetCurrent()
{
	auto id = processorGetCurrentId();
	g_processor* processor = processors;
	while(processor && id--)
	{
		processor = processor->next;
	}
	return processor;
}

void processorInitializeBsp()
{
	processorPrintInformation();

	if(!processorHasFeature(g_cpuid_standard_edx_feature::APIC))
		panic("%! processor has no APIC", "cpu");
}

void processorFinalizeSetup()
{
	if(processorHasFeature(g_cpuid_standard_edx_feature::SSE))
	{
		_enableSSE();
		auto core = _processorGetCurrent();
		core->sseReady = true;
		logDebug("%! %i: SSE2 support enabled", "cpu", processorGetCurrentId());

		// TODO Allocator not capable of aligned allocation
		core->fpu.initialStateMem = (uint8_t*) heapAllocate(G_SSE_STATE_SIZE + G_SSE_STATE_ALIGNMENT);
		core->fpu.initialState = (uint8_t*) G_ALIGN_UP((g_address) core->fpu.initialStateMem, G_SSE_STATE_ALIGNMENT);
		processorSaveFpuState(core->fpu.initialState);
	}
	else
	{
		logWarn("%! no SSE support", "cpu");
	}
}

bool processorHasFeatureReady(g_cpuid_standard_edx_feature feature)
{
	auto processor = _processorGetCurrent();
	if(!processor)
		panic("%! tried to check for processor feature while not ready", "cpu");

	if(feature == g_cpuid_standard_edx_feature::SSE || feature == g_cpuid_standard_edx_feature::SSE2)
		return processor->sseReady;

	return false;
}

void processorApicIdCreateMappingTable()
{
	uint32_t highestApicId = 0;
	g_processor* p = processors;
	while(p)
	{
		if(p->apicId > highestApicId)
		{
			highestApicId = p->apicId;
		}
		p = p->next;
	}

	if(highestApicId > 1024)
		panic("%! weirdly high apic id detected: %i", "cpu", highestApicId);

	uint32_t mappingSize = sizeof(uint32_t) * (highestApicId + 1);
	uint32_t* mapping = (uint32_t*) heapAllocate(mappingSize);
	memorySetBytes((void*) mapping, 0, mappingSize);
	p = processors;
	while(p)
	{
		mapping[p->apicId] = p->id;
		p = p->next;
	}
	apicIdToProcessorMapping = mapping;
}

void processorAdd(uint32_t apicId, uint32_t processorHardwareId)
{
	g_processor* existing = processors;
	while(existing)
	{
		if(existing->apicId == apicId)
		{
			logWarn("%! ignoring core with irregular, duplicate apic id %i", "system", apicId);
			return;
		}
		existing = existing->next;
	}

	auto core = (g_processor*) heapAllocate(sizeof(g_processor));
	core->id = processorsAvailable;
	core->hardwareId = processorHardwareId;
	core->apicId = apicId;
	core->next = processors;

	// BSP executes this code
	if(!lapicIsAvailable() || apicId == lapicReadId())
	{
		core->bsp = true;
	}

	processors = core;

	++processorsAvailable;
}

uint16_t processorGetNumberOfProcessors()
{
	if(!G_SMP_ENABLED)
		return 1;

	if(!processors)
		panic("%! tried to retrieve number of cores before initializing system on BSP", "kern");
	return processorsAvailable;
}

uint32_t processorGetCurrentId()
{
	// While system is not ready, read the processor ID from the APIC
	// which is very slow but works for the time being.
	if(!systemIsReady())
		return processorGetCurrentIdFromApic();

	// GS points to valid <g_kernel_threadlocal>
	// 0x0 is relative address within struct
	uint64_t processor;
	asm volatile("mov %%gs:0x0, %0" : "=r" (processor));
	return processor;
}

uint32_t processorGetCurrentIdFromApic()
{
	if(!apicIdToProcessorMapping)
		return 0;
	return apicIdToProcessorMapping[lapicReadId()];
}

bool processorListAvailable()
{
	return processors != nullptr;
}

void processorCpuid(uint32_t code, uint32_t* outA, uint32_t* outB, uint32_t* outC, uint32_t* outD)
{
	asm volatile("cpuid"
		: "=a"(*outA), "=b"(*outB), "=c"(*outC), "=d"(*outD)
		: "a"(code));
}

bool processorHasFeature(g_cpuid_standard_edx_feature feature)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	processorCpuid(1, &eax, &ebx, &ecx, &edx);
	return (edx & (uint64_t) feature);
}

bool processorHasFeature(g_cpuid_extended_ecx_feature feature)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	processorCpuid(1, &eax, &ebx, &ecx, &edx);
	return (ecx & (uint64_t) feature);
}

void processorGetVendor(char* out)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	processorCpuid(0, &eax, &ebx, &ecx, &edx);

	uint32_t* io = (uint32_t*) out;
	io[0] = ebx;
	io[1] = edx;
	io[2] = ecx;
}

void processorPrintInformation()
{
	char vendor[13];
	processorGetVendor(vendor);
	vendor[12] = 0;
	logInfo("%! vendor: '%s'", "cpu", vendor);

	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	processorCpuid(1, &eax, &ebx, &ecx, &edx);

	logInfon("%! available features:", "cpu");
	if(edx & (int64_t) g_cpuid_standard_edx_feature::PAE)
	{
		logInfon(" PAE");
	}
	if(edx & (int64_t) g_cpuid_standard_edx_feature::MMX)
	{
		logInfon(" MMX");
	}
	if(edx & (int64_t) g_cpuid_standard_edx_feature::SSE)
	{
		logInfon(" SSE");
	}
	if(edx & (int64_t) g_cpuid_standard_edx_feature::SSE2)
	{
		logInfon(" SSE2");
	}
	logInfo("");
}

g_processor* processorGetList()
{
	return processors;
}

void processorReadMsr(uint32_t msr, uint32_t* lo, uint32_t* hi)
{
	asm volatile("rdmsr"
		: "=a"(*lo), "=d"(*hi)
		: "c"(msr));
}

void processorWriteMsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
	asm volatile("wrmsr"
		:
		: "a"(lo), "d"(hi), "c"(msr));
}

uint64_t processorReadEflags()
{
	uint64_t eflags;
	asm volatile("pushf\n"
		"pop %0"
		: "=g"(eflags));
	return eflags;
}

void processorSaveFpuState(uint8_t* target)
{
	asm volatile (
		"fxsave (%0)"
		:
		: "r" (target)
		: "memory"
	);
}

void processorRestoreFpuState(uint8_t* source)
{
	asm volatile (
		"fxrstor (%0)"
		:
		: "r" (source)
		: "memory"
	);
}

const uint8_t* processorGetInitialFpuState()
{
	return _processorGetCurrent()->fpu.initialState;
}

bool processorIsBsp()
{
	return processorGetCurrentId() == 0;
}

uint64_t processorReadTsc()
{
	uint32_t lo, hi;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t) hi << 32) | lo;
}
