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
#define GHOST_API_STDINT
#include <ghost/system/acpi.h>

namespace {
struct g_task { int securityLevel; } driver{1}, application{2};
uint64_t acpiPlatformRsdp, mappingResult;
bool ioapicAvailable, readable;
size_t readableBytes;
unsigned copies, maps;
uint64_t mappedAddress;
uint32_t mappedSize;

bool memoryCopyFromUser(g_task*, void* output, g_address input, size_t size)
{
	if(!readable || size > readableBytes) return false;

	memcpy(output, (void*) input, size);
	return true;
}

bool memoryCopyToUser(g_task*, g_address output, const void* input, size_t size)
{
	++copies;
	memcpy((void*) output, input, size);
	return true;
}

bool ioapicAreAvailable() { return ioapicAvailable; }
uint64_t acpiPlatformGetRsdp() { return acpiPlatformRsdp; }
uint64_t acpiPlatformMap(g_task*, uint64_t address, uint32_t size)
{
	++maps;
	mappedAddress = address;
	mappedSize = size;
	return mappingResult;
}

#include "support/memory_user.hpp"
#include "production/acpi_platform_syscall.inc"
#include "support/syscall_dispatch.hpp"

class AcpiPlatform : public testing::Test
{
	void SetUp() override
	{
		acpiPlatformRsdp = 0x12345000;
		mappingResult = 0x9000;
		ioapicAvailable = readable = true;
		readableBytes = SIZE_MAX;
		copies = maps = 0;
		mappedAddress = mappedSize = 0;
	}
};

TEST_F(AcpiPlatform, HardwareOperationsRequireDriverPrivilege)
{
	g_syscall_acpi_platform_info info{123, 1, G_ACPI_SUCCESSFUL};
	syscallTestInvoke(&application, (g_syscall_handler) syscallAcpiGetPlatformInfo, &info, sizeof(info), G_SYSCALL_OUTPUT);

	EXPECT_EQ(G_ACPI_NOT_PERMITTED, info.status);
	EXPECT_EQ(0u, uint64_t(info.rsdp));
	EXPECT_EQ(0, info.ioapic);

	g_syscall_acpi_map mapping{0x2003, 17, 123, G_ACPI_SUCCESSFUL};
	syscallTestInvoke(&application, (g_syscall_handler) syscallAcpiMap, &mapping, sizeof(mapping));

	EXPECT_EQ(G_ACPI_NOT_PERMITTED, mapping.status);
	EXPECT_EQ(0u, uint64_t(mapping.virtualAddress));
	EXPECT_EQ(0u, maps);
}

TEST_F(AcpiPlatform, ReturnsPlatformDataAndKeepsMappingInputSeparateFromItsResult)
{
	g_syscall_acpi_platform_info info{};
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiGetPlatformInfo, &info, sizeof(info), G_SYSCALL_OUTPUT);

	EXPECT_EQ(G_ACPI_SUCCESSFUL, info.status);
	EXPECT_EQ(acpiPlatformRsdp, uint64_t(info.rsdp));
	EXPECT_EQ(1, info.ioapic);

	g_syscall_acpi_map mapping{0x2003, 17, 0, G_ACPI_NOT_SUPPORTED};
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiMap, &mapping, sizeof(mapping));

	EXPECT_EQ(G_ACPI_SUCCESSFUL, mapping.status);
	EXPECT_EQ(0x9000u, uint64_t(mapping.virtualAddress));
	EXPECT_EQ(0x2003u, uint64_t(mapping.physicalAddress));
	EXPECT_EQ(0x2003u, mappedAddress);
	EXPECT_EQ(17u, mappedSize);
}

TEST_F(AcpiPlatform, ReportsUnavailableFirmwareAndFailedMappings)
{
	acpiPlatformRsdp = 0;
	ioapicAvailable = false;
	g_syscall_acpi_platform_info info{};
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiGetPlatformInfo, &info, sizeof(info), G_SYSCALL_OUTPUT);

	EXPECT_EQ(G_ACPI_NOT_SUPPORTED, info.status);
	EXPECT_EQ(0u, uint64_t(info.rsdp));
	EXPECT_EQ(0, info.ioapic);

	mappingResult = 0;
	g_syscall_acpi_map mapping{0x2003, 17, 123, G_ACPI_SUCCESSFUL};
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiMap, &mapping, sizeof(mapping));

	EXPECT_EQ(G_ACPI_INVALID_ARGUMENT, mapping.status);
	EXPECT_EQ(0u, uint64_t(mapping.virtualAddress));
}

TEST_F(AcpiPlatform, RejectsInaccessibleRequestsBeforeHardwareAccess)
{
	g_syscall_acpi_map request{0x2003, 17, 0, G_ACPI_NOT_SUPPORTED};
	readable = false;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiMap, &request, sizeof(request));

	readable = true;
	readableBytes = sizeof(request) - 1;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiMap, &request, sizeof(request));

	g_syscall_acpi_platform_info info{};
	readableBytes = sizeof(info) - 1;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallAcpiGetPlatformInfo, &info, sizeof(info), G_SYSCALL_OUTPUT);

	EXPECT_EQ(0u, copies);
	EXPECT_EQ(0u, maps);
}
}
