#include "support/fixture.hpp"
#define GHOST_API_STDINT
#include <ghost/system/acpi.h>

namespace {
constexpr int G_SECURITY_LEVEL_DRIVER = 1;
struct g_task { int securityLevel; } driver{1}, application{2};
uint64_t acpiPlatformRsdp;
bool ioapicAvailable, readable;
unsigned copies, maps, waits, configurations;
uint64_t mappedAddress;
uint32_t configuredIrq, mappedSize;
uint8_t configuredOperation;

bool memoryCopyFromUser(g_task*, void* output, g_address input, size_t size)
{
	if(!readable) return false;
	memcpy(output, reinterpret_cast<void*>(input), size);
	return true;
}

bool memoryCopyToUser(g_task*, g_address output, const void* input, size_t size)
{
	++copies;
	memcpy(reinterpret_cast<void*>(output), input, size);
	return true;
}

bool ioapicAreAvailable() { return ioapicAvailable; }
uint64_t acpiPlatformGetRsdp() { return acpiPlatformRsdp; }
uint64_t acpiPlatformMap(g_task*, uint64_t address, uint32_t size)
{
	++maps;
	mappedAddress = address;
	mappedSize = size;
	return 0x9000;
}
g_acpi_status acpiPlatformWait(g_task*, uint32_t irq)
{
	++waits;
	configuredIrq = irq;
	return G_ACPI_BUSY;
}
g_acpi_status acpiTestConfigureIrq(uint32_t irq, uint8_t operation)
{
	++configurations;
	configuredIrq = irq;
	configuredOperation = operation;
	return G_ACPI_SUCCESSFUL;
}
g_acpi_status acpiPlatformInstallIrq(g_task*, uint32_t irq) { return acpiTestConfigureIrq(irq, G_ACPI_PLATFORM_IRQ_INSTALL); }
g_acpi_status acpiPlatformRearmIrq(g_task*, uint32_t irq) { return acpiTestConfigureIrq(irq, G_ACPI_PLATFORM_IRQ_REARM); }
g_acpi_status acpiPlatformRemoveIrq(g_task*, uint32_t irq) { return acpiTestConfigureIrq(irq, G_ACPI_PLATFORM_IRQ_REMOVE); }
g_acpi_status acpiPlatformMaskIrq(g_task*, uint32_t irq) { return acpiTestConfigureIrq(irq, G_ACPI_PLATFORM_IRQ_MASK); }

#include "production/acpi_platform_syscall.inc"

class AcpiPlatform : public testing::Test
{
	void SetUp() override
	{
		acpiPlatformRsdp = 0x12345000;
		ioapicAvailable = readable = true;
		copies = maps = waits = configurations = 0;
		mappedAddress = mappedSize = configuredIrq = configuredOperation = 0;
	}
};

TEST_F(AcpiPlatform, HardwareOperationsRequireDriverPrivilege)
{
	for(uint8_t operation = G_ACPI_PLATFORM_INFO; operation <= G_ACPI_PLATFORM_IRQ_MASK; ++operation)
	{
		g_syscall_acpi_platform request{};
		request.operation = operation;
		syscallAcpiPlatform(&application, &request);
		EXPECT_EQ(G_ACPI_NOT_PERMITTED, request.status);
	}
	EXPECT_EQ(0u, maps + waits + configurations);
}

TEST_F(AcpiPlatform, ReturnsPlatformDataAndDispatchesTheRemainingOperations)
{
	g_syscall_acpi_platform request{};
	request.operation = G_ACPI_PLATFORM_INFO;
	syscallAcpiPlatform(&driver, &request);
	EXPECT_EQ(G_ACPI_SUCCESSFUL, request.status);
	EXPECT_EQ(acpiPlatformRsdp, request.address);
	EXPECT_EQ(1, request.ioapic);

	request.operation = G_ACPI_PLATFORM_MAP;
	request.address = 0x2003;
	request.size = 17;
	syscallAcpiPlatform(&driver, &request);
	EXPECT_EQ(G_ACPI_SUCCESSFUL, request.status);
	EXPECT_EQ(0x9000u, request.address);
	EXPECT_EQ(0x2003u, mappedAddress);
	EXPECT_EQ(17u, mappedSize);

	request.operation = G_ACPI_PLATFORM_IRQ_WAIT;
	request.irq = 9;
	syscallAcpiPlatform(&driver, &request);
	EXPECT_EQ(G_ACPI_BUSY, request.status);
	EXPECT_EQ(1u, waits);
	EXPECT_EQ(9u, configuredIrq);

	for(auto operation : {G_ACPI_PLATFORM_IRQ_INSTALL, G_ACPI_PLATFORM_IRQ_REARM,
		G_ACPI_PLATFORM_IRQ_REMOVE, G_ACPI_PLATFORM_IRQ_MASK})
	{
		request.operation = operation;
		syscallAcpiPlatform(&driver, &request);
		EXPECT_EQ(G_ACPI_SUCCESSFUL, request.status);
		EXPECT_EQ(operation, configuredOperation);
	}
	EXPECT_EQ(4u, configurations);
}

TEST_F(AcpiPlatform, RejectsTheRemovedDiscoveryOperationAndUnreadableRequests)
{
	g_syscall_acpi_platform request{};
	request.status = G_ACPI_SUCCESSFUL;
	syscallAcpiPlatform(&driver, &request);
	EXPECT_EQ(G_ACPI_INVALID_ARGUMENT, request.status);
	EXPECT_EQ(0u, maps + waits + configurations);

	readable = false;
	copies = 0;
	request.operation = G_ACPI_PLATFORM_MAP;
	syscallAcpiPlatform(&driver, &request);
	EXPECT_EQ(0u, copies);
	EXPECT_EQ(0u, maps);
}
}
