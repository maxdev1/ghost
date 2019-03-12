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
#include "kernel/kernel.hpp"
#include "shared/logger/logger.hpp"

static g_processor* processors = 0;
static uint32_t processorsAvailable = 0;

static uint32_t* apicIdToProcessorMapping = 0;

void processorInitializeBsp()
{
	if(!processorSupportsCpuid())
		kernelPanic("%! processor has no CPUID support", "cpu");

	processorPrintInformation();
	processorEnableSSE();

	if(!processorHasFeature(g_cpuid_standard_edx_feature::APIC))
		kernelPanic("%! processor has no APIC", "cpu");
}

void processorInitializeAp()
{
	processorEnableSSE();
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
		kernelPanic("%! weirdly high apic id detected: %i", "cpu", highestApicId);

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

	g_processor* core = (g_processor*) heapAllocate(sizeof(g_processor));
	core->id = processorsAvailable;
	core->hardwareId = processorHardwareId;
	core->apicId = apicId;
	core->next = processors;

	// BSP executes this code
	if(apicId == lapicReadId())
	{
		core->bsp = true;
	}

	processors = core;

	++processorsAvailable;
}

uint16_t processorGetNumberOfProcessors()
{
	if(!processors)
		kernelPanic("%! tried to retrieve number of cores before initializing system on BSP", "kern");
	return processorsAvailable;
}

uint32_t processorGetCurrentId()
{
	if(apicIdToProcessorMapping == 0)
		return 0;
	return apicIdToProcessorMapping[lapicReadId()];
}

bool processorListAvailable()
{
	return processors != 0;
}

bool processorSupportsCpuid()
{
	return _checkForCPUID();
}

void processorCpuid(uint32_t code, uint32_t* outA, uint32_t* outB, uint32_t* outC, uint32_t* outD)
{
	asm volatile("cpuid" : "=a"(*outA), "=b"(*outB), "=c"(*outC), "=d"(*outD) : "a"(code));
}

void processorEnableSSE()
{
	if(processorHasFeature(g_cpuid_standard_edx_feature::SSE))
	{
		_enableSSE();
		logDebug("%! support enabled", "sse");
	} else
	{
		logWarn("%! not supported", "sse");
	}
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

void processorReadMsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void processorWriteMsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
	asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

uint32_t processorReadEflags() {
    uint32_t eflags;
    asm volatile("pushf\n"
                	"pop %0"
                   : "=g"(eflags));
	return eflags;
}
