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

#include <cstring>
#include <ghost.h>
#include <ghost/system/pci.h>
#include <uacpi/kernel_api.h>
#include "platform.hpp"

static constexpr uint64_t G_ACPI_PCI_TIMEOUT_MS = 15000;
static g_tid acpiPciService = G_TID_NONE;

bool acpiPlatformPciInitialize()
{
	auto deadline = g_millis() + G_ACPI_TIMEOUT_MS;
	while((acpiPciService = g_task_get_by_name(G_PCI_DRIVER_NAME)) == G_TID_NONE)
	{
		if(g_millis() >= deadline)
			return false;
		g_sleep(1);
	}
	return true;
}

extern "C" uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size length, uacpi_handle* out)
{
	if(base > 0xffff || !length || length > 0x10000 - base)
		return UACPI_STATUS_INVALID_ARGUMENT;
	*out = reinterpret_cast<void*>(base);
	return UACPI_STATUS_OK;
}

extern "C" void uacpi_kernel_io_unmap(uacpi_handle) {}

#define ACPI_IO(bits, type, read, write)                                                                               \
	extern "C" uacpi_status uacpi_kernel_io_read##bits(uacpi_handle base, uacpi_size offset, type* value)              \
	{                                                                                                                  \
		*value = read(reinterpret_cast<uintptr_t>(base) + offset);                                                     \
		return UACPI_STATUS_OK;                                                                                        \
	}                                                                                                                  \
	extern "C" uacpi_status uacpi_kernel_io_write##bits(uacpi_handle base, uacpi_size offset, type value)              \
	{                                                                                                                  \
		write(reinterpret_cast<uintptr_t>(base) + offset, value);                                                      \
		return UACPI_STATUS_OK;                                                                                        \
	}

ACPI_IO(8, uacpi_u8, g_io_port_read_byte, g_io_port_write_byte)
ACPI_IO(16, uacpi_u16, g_io_port_read_word, g_io_port_write_word)
ACPI_IO(32, uacpi_u32, g_io_port_read_dword, g_io_port_write_dword)

static bool acpiPlatformPciTransact(void* request, size_t length, void* response, size_t responseSize)
{
	auto tx = g_get_message_tx_id();
	auto deadline = g_millis() + G_ACPI_PCI_TIMEOUT_MS;
	while(g_send_message_tm(acpiPciService, request, length, tx, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
		  G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		if(g_millis() >= deadline || g_task_get_by_name(G_PCI_DRIVER_NAME) != acpiPciService)
			return false;
		g_sleep(1);
	}

	uint8_t bytes[sizeof(g_message_header) + sizeof(g_pci_read_config_response)];
	while(g_millis() < deadline)
	{
		if(g_task_get_by_name(G_PCI_DRIVER_NAME) != acpiPciService)
			return false;

		if(g_receive_message_tm(bytes, sizeof(bytes), tx, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) ==
		   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto header = reinterpret_cast<g_message_header*>(bytes);
			if(header->sender == acpiPciService && header->length == responseSize)
			{
				memcpy(response, G_MESSAGE_CONTENT(bytes), responseSize);
				return true;
			}
		}

		g_sleep(1);
	}
	return false;
}

static bool acpiPlatformPciRead(uint32_t device, uint8_t offset, uint8_t width, uint32_t* value)
{
	g_pci_read_config_request request{{G_PCI_READ_CONFIG}, device, offset, width};
	g_pci_read_config_response response{};
	if(!acpiPlatformPciTransact(&request, sizeof(request), &response, sizeof(response)) || !response.successful)
		return false;

	*value = response.value;
	return true;
}

static bool acpiPlatformPciWrite(uint32_t device, uint8_t offset, uint8_t width, uint32_t value)
{
	g_pci_write_config_request request{{G_PCI_WRITE_CONFIG}, device, offset, width, value};
	g_pci_write_config_response response{};
	return acpiPlatformPciTransact(&request, sizeof(request), &response, sizeof(response)) && response.successful;
}

extern "C" uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle* out)
{
	if(address.segment || address.device > 31 || address.function > 7)
		return UACPI_STATUS_UNIMPLEMENTED;

	auto device = G_PCI_DEVICE_ADDRESS_BUILD(address.bus, address.device, address.function);
	uint32_t vendor = 0;

	if(!acpiPlatformPciRead(device, 0, 2, &vendor) || vendor == 0xffff)
		return UACPI_STATUS_NOT_FOUND;

	*out = reinterpret_cast<void*>(uintptr_t(device) + 1);
	return UACPI_STATUS_OK;
}

extern "C" void uacpi_kernel_pci_device_close(uacpi_handle) {}
#define ACPI_PCI(bits, type)                                                                                           \
	extern "C" uacpi_status uacpi_kernel_pci_read##bits(uacpi_handle handle, uacpi_size offset, type* value)           \
	{                                                                                                                  \
		if(offset > 256 - sizeof(type) || (offset & (sizeof(type) - 1)))                                               \
			return UACPI_STATUS_INVALID_ARGUMENT;                                                                      \
		uint32_t result = 0;                                                                                           \
		bool ok = acpiPlatformPciRead(reinterpret_cast<uintptr_t>(handle) - 1, offset, sizeof(type), &result);         \
		*value = result;                                                                                               \
		return ok ? UACPI_STATUS_OK : UACPI_STATUS_INTERNAL_ERROR;                                                     \
	}                                                                                                                  \
	extern "C" uacpi_status uacpi_kernel_pci_write##bits(uacpi_handle handle, uacpi_size offset, type value)           \
	{                                                                                                                  \
		if(offset > 256 - sizeof(type) || (offset & (sizeof(type) - 1)))                                               \
			return UACPI_STATUS_INVALID_ARGUMENT;                                                                      \
		return acpiPlatformPciWrite(reinterpret_cast<uintptr_t>(handle) - 1, offset, sizeof(type), value)              \
					   ? UACPI_STATUS_OK                                                                               \
					   : UACPI_STATUS_INTERNAL_ERROR;                                                                  \
	}

ACPI_PCI(8, uacpi_u8)
ACPI_PCI(16, uacpi_u16)
ACPI_PCI(32, uacpi_u32)
