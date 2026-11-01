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

#include <ahcidriver_platform.hpp>
#include <cstdio>
#include <libdevice/manager.hpp>
#include <libpci/driver.hpp>
#include <vector>
#include "ahcidriver.hpp"

static constexpr uint32_t G_AHCI_PCI_MEMORY_SPACE = 1 << 1;
static constexpr uint32_t G_AHCI_PCI_BUS_MASTER = 1 << 2;
static constexpr uint32_t G_AHCI_PCI_DISABLE_INTERRUPTS = 1 << 10;
static constexpr uint32_t G_AHCI_CONTROLLER_ENABLE = 1u << 31;
static constexpr uint32_t G_AHCI_CONTROLLER_INTERRUPTS = 1 << 1;
static constexpr uint32_t G_AHCI_CAP_64BIT = 1u << 31;
static constexpr uint32_t G_AHCI_CAP2_HANDOFF = 1;
static constexpr uint32_t G_AHCI_HANDOFF_OS_OWNED = 1 << 1;
static constexpr uint32_t G_AHCI_HANDOFF_FIRMWARE_BUSY = 1 | (1 << 4);
static constexpr size_t G_AHCI_REGISTER_BYTES = 0x1100;
static std::vector<g_ahci_device*> ahciDisks;

static volatile g_ahci_hba_ghc* ahciMapController(uint32_t bar, uint8_t** mapping)
{
	uintptr_t physical = bar & ~uintptr_t(15);
	uintptr_t offset = physical & (G_PAGE_SIZE - 1);
	*mapping = static_cast<uint8_t*>(
			g_map_mmio(reinterpret_cast<void*>(physical - offset), offset + G_AHCI_REGISTER_BYTES));
	return *mapping ? reinterpret_cast<volatile g_ahci_hba_ghc*>(*mapping + offset) : nullptr;
}

static bool ahciAcquireController(volatile g_ahci_hba_ghc* hba)
{
	if(ahciReadRegister(&hba->cap2) & G_AHCI_CAP2_HANDOFF)
	{
		ahciWriteRegister(&hba->bohc, ahciReadRegister(&hba->bohc) | G_AHCI_HANDOFF_OS_OWNED);
		if(!ahciWaitRegisterClear(&hba->bohc, G_AHCI_HANDOFF_FIRMWARE_BUSY, G_AHCI_COMMAND_TIMEOUT_MS))
		{
			klog("ahci: firmware ownership handoff timed out");
			return false;
		}
	}
	ahciWriteRegister(&hba->ghc,
					  (ahciReadRegister(&hba->ghc) | G_AHCI_CONTROLLER_ENABLE) & ~G_AHCI_CONTROLLER_INTERRUPTS);
	return true;
}

/**
 * Maps all 32 ports, including a BAR's page offset. Once port initialization
 * begins, the mapping stays for the service lifetime because failed ports may
 * still DMA.
 */
static bool ahciInitializeController(const g_pci_device_data& pci)
{
	uint32_t bar, command;
	if(!pciDriverReadConfig(pci.deviceAddress, PCI_CONFIG_OFF_BAR5, 4, &bar) || !bar || (bar & 7) ||
	   !pciDriverReadConfig(pci.deviceAddress, PCI_CONFIG_OFF_COMMAND, 2, &command) ||
	   !pciDriverWriteConfig(pci.deviceAddress, PCI_CONFIG_OFF_COMMAND, 2,
							 command | G_AHCI_PCI_MEMORY_SPACE | G_AHCI_PCI_BUS_MASTER | G_AHCI_PCI_DISABLE_INTERRUPTS))
		return false;

	uint8_t* mapping;
	auto hba = ahciMapController(bar, &mapping);
	if(!hba)
		return false;
	if(!ahciAcquireController(hba))
	{
		g_unmap(mapping);
		return false;
	}

	auto ports = reinterpret_cast<volatile g_ahci_hba_port*>(reinterpret_cast<volatile uint8_t*>(hba) +
															 G_AHCI_HBA_PORT_OFFSET);
	uint32_t implemented = ahciReadRegister(&hba->pi);
	klog("ahci: controller %x BAR %x ports %x", pci.deviceAddress, bar, implemented);
	bool supports64BitDma = ahciReadRegister(&hba->cap) & G_AHCI_CAP_64BIT;
	for(uint8_t number = 0; number < 32; ++number)
	{
		if(!(implemented & (1u << number)))
			continue;
		auto disk = ahciInitializeDisk(number, &ports[number], supports64BitDma);
		if(disk)
			ahciDisks.push_back(disk);
	}
	return true;
}

g_ahci_device* ahciFindDisk(g_device_id id)
{
	for(auto disk: ahciDisks)
		if(disk->id == id)
			return disk;
	return nullptr;
}

bool ahciInitializeControllers()
{
	int count = 0;
	g_pci_device_data* pci = nullptr;
	if(!pciDriverListDevices(&count, &pci))
		return false;

	for(int i = 0; i < count; ++i)
	{
		if(pci[i].classCode == PCI_BASE_CLASS_MASS_STORAGE && pci[i].subclassCode == PCI_01_SUBCLASS_SATA &&
		   pci[i].progIf == PCI_01_06_PROGIF_AHCI && !ahciInitializeController(pci[i]))
			klog("ahci: failed to initialize controller %x", pci[i].deviceAddress);
	}

	pciDriverFreeDeviceList(pci);
	return true;
}

bool ahciPowerFlush()
{
	for(auto disk: ahciDisks)
	{
		if(!disk->online || !disk->dmaSafe)
			return false;
		if(disk->flushCommand && ahciExecuteCommand(disk, disk->flushCommand, 0, 0, 0, false) != G_BLOCK_SUCCESS)
			return false;
	}
	return true;
}

void ahciRegisterDevices()
{
	for(auto disk: ahciDisks)
	{
		g_device_id id;
		if(!deviceManagerRegisterDevice(G_DEVICE_TYPE_BLOCK, g_get_tid(), &id))
		{
			disk->online = false;
			klog("ahci: failed to register port %i", disk->portNumber);
			continue;
		}
		disk->id = id;
		klog("ahci: block device %i port %i model %s sectors %lu sector-size %i", id, disk->portNumber,
			 disk->info.model, disk->info.sectorCount, disk->info.sectorSize);
	}
}
