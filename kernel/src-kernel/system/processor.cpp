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

#include <logger/logger.hpp>
#include <kernel.hpp>
#include <system/processor.hpp>

/**
 *
 */
bool g_processor::supportsCpuid() {
	return _checkForCPUID();
}

/**
 *
 */
void g_processor::cpuid(uint32_t code, uint32_t* outA, uint32_t* outB, uint32_t* outC, uint32_t* outD) {
	asm volatile("cpuid" : "=a"(*outA), "=b"(*outB), "=c"(*outC), "=d"(*outD) : "a"(code));
}

/**
 *
 */
void g_processor::enableSSE() {
	_enableSSE();
}

/**
 *
 */
bool g_processor::hasFeature(g_cpuid_standard_edx_feature feature) {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	cpuid(1, &eax, &ebx, &ecx, &edx);
	return (edx & (uint64_t) feature);
}

/**
 *
 */
bool g_processor::hasFeature(g_cpuid_extended_ecx_feature feature) {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	cpuid(1, &eax, &ebx, &ecx, &edx);
	return (ecx & (uint64_t) feature);
}

/**
 *
 */
void g_processor::getVendor(char* out) {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	cpuid(0, &eax, &ebx, &ecx, &edx);

	uint32_t* io = (uint32_t*) out;
	io[0] = ebx;
	io[1] = edx;
	io[2] = ecx;
}

/**
 *
 */
void g_processor::printInformation() {
	char vendor[13];
	g_processor::getVendor(vendor);
	vendor[12] = 0;
	g_log_info("%! vendor: '%s'", "cpu", vendor);

	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	cpuid(1, &eax, &ebx, &ecx, &edx);

	g_log_infon("%! cpu features:", "cpu");
	if (edx & (int64_t) g_cpuid_standard_edx_feature::PAE) {
		g_log_infon(" PAE");
	}
	if (edx & (int64_t) g_cpuid_standard_edx_feature::MMX) {
		g_log_infon(" MMX");
	}
	if (edx & (int64_t) g_cpuid_standard_edx_feature::SSE) {
		g_log_infon(" SSE");
	}
	if (edx & (int64_t) g_cpuid_standard_edx_feature::SSE2) {
		g_log_infon(" SSE2");
	}
	g_log_info("");
}

/**
 * 
 */
void g_processor::readMsr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
	asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

/**
 * 
 */
void g_processor::writeMsr(uint32_t msr, uint32_t lo, uint32_t hi) {
	asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

