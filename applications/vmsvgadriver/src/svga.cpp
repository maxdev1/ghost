/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "svga.hpp"
#include <libpci/driver.hpp>
#include <cstdio>
#include <malloc.h>
#include <libwindow/color_argb.hpp>

svga_device_t device;

bool svgaGetPciControllerData()
{
	int count;
	g_pci_device_data* devices;
	if(!pciDriverListDevices(&count, &devices))
	{
		klog("failed to list PCI devices");
		return false;
	}

	g_pci_device_address address;
	bool found = false;
	for(int i = 0; i < count; i++)
	{
		if(devices[i].classCode == PCI_BASE_CLASS_DISPLAY &&
		   devices[i].subclassCode == PCI_03_SUBCLASS_VGA &&
		   devices[i].progIf == PCI_03_00_PROGIF_VGA_COMPATIBLE)
		{
			uint32_t vendorId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId))
			{
				klog("Failed to read vendor ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			uint32_t deviceId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_DEVICE_ID, 2, &deviceId))
			{
				klog("Failed to read device ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			if(vendorId == 0x15AD /* VMWare */ && deviceId == 0x0405 /* SVGA2 */)
			{
				address = devices[i].deviceAddress;
				found = true;
				break;
			}
		}
	}
	pciDriverFreeDeviceList(devices);

	if(found)
	{
		if(!pciDriverEnableResourceAccess(address, true))
		{
			klog("failed to enable resource access of VMSVGA controller");
			return false;
		}
		if(!pciDriverReadBAR(address, 0, &device.ioBase))
		{
			klog("failed to read BAR0 of VMSVGA controller");
			return false;
		}
		if(!pciDriverReadBAR(address, 1, &device.fb.physical))
		{
			klog("failed to read BAR1 of VMSVGA controller");
			return false;
		}
		if(!pciDriverReadBAR(address, 2, &device.fifo.physical))
		{
			klog("failed to read BAR2 of VMSVGA controller");
			return false;
		}
	}
	return found;
}

bool svgaInitializeDevice()
{
	if(!svgaGetPciControllerData())
		return false;
	device.vramSize = svgaReadReg(SVGA_REG_VRAM_SIZE);

	if(!svgaIdentifyVersion())
	{
		klog("failed to identify SVGA version");
		return false;
	}
	klog("device version: %x", device.versionId);

	device.fifo.size = svgaReadReg(SVGA_REG_MEM_SIZE);
	device.fifo.mapped = (uint32_t*) g_map_mmio((void*) device.fifo.physical, device.fifo.size);
	device.fifo.mapped[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
	device.fifo.mapped[SVGA_FIFO_MAX] = device.fifo.size;
	device.fifo.mapped[SVGA_FIFO_NEXT_CMD] = device.fifo.mapped[SVGA_FIFO_MIN];
	device.fifo.mapped[SVGA_FIFO_STOP] = device.fifo.mapped[SVGA_FIFO_MIN];

	bool reserveable = svgaFifoHasCapability(SVGA_FIFO_CAP_RESERVE);
	if(!reserveable)
	{
		klog("Error: SVGA FIFO does not have reserve capability");
		return false;
	}

	return true;
}

bool svgaIdentifyVersion()
{
	do
	{
		svgaWriteReg(SVGA_REG_ID, device.versionId);
		if(svgaReadReg(SVGA_REG_ID) == device.versionId)
			break;
		device.versionId--;
	} while(device.versionId >= SVGA_ID_0);

	return device.versionId >= SVGA_ID_0;
}


void svgaSetMode(uint32_t width, uint32_t height, uint32_t bpp)
{
	svgaWriteReg(SVGA_REG_WIDTH, width);
	svgaWriteReg(SVGA_REG_HEIGHT, height);
	svgaWriteReg(SVGA_REG_BITS_PER_PIXEL, bpp);
	svgaWriteReg(SVGA_REG_ENABLE, true);
	svgaWriteReg(SVGA_REG_CONFIG_DONE, true);

	device.fb.size = svgaReadReg(SVGA_REG_FB_SIZE);
	device.fb.mapped = (uint32_t*) g_map_mmio((void*) device.fb.physical, device.fb.size);
}

uint32_t* svgaGetFb()
{
	return device.fb.mapped;
}

uint32_t svgaGetFbSize()
{
	return device.fb.size;
}

void svgaUpdate(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	auto cmd = (SVGAFifoCmdUpdate*) svgaFifoReserveCommand(SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	svgaFifoCommitReserved();
}

void svgaFifoCommitReserved()
{
	svgaFifoCommit(device.fifo.reservedSize);
}

void* svgaFifoReserveSpace(uint32_t bytes)
{
	uint32_t max = device.fifo.mapped[SVGA_FIFO_MAX];
	uint32_t min = device.fifo.mapped[SVGA_FIFO_MIN];
	uint32_t nextCmd = device.fifo.mapped[SVGA_FIFO_NEXT_CMD];

	if(bytes > (max - min))
	{
		klog("error: FIFO command too large");
		return nullptr;
	}

	device.fifo.reservedSize = bytes;

	uint32_t stop = device.fifo.mapped[SVGA_FIFO_STOP];
	bool hasSpace = false;

	if(nextCmd >= stop)
	{
		if(nextCmd + bytes < max ||
		   (nextCmd + bytes == max && stop > min))
		{
			hasSpace = true;
		}
	}
	else if(nextCmd + bytes < stop)
	{
		hasSpace = true;
	}

	if(hasSpace)
	{
		device.fifo.mapped[SVGA_FIFO_RESERVED] = bytes;
		return nextCmd + (uint8_t*) device.fifo.mapped;
	}

	klog("Error: SVGA FIFO is full");
	return nullptr;
}

void* svgaFifoReserveCommand(uint32_t type, uint32_t bytes)
{
	auto cmd = (uint32_t*) svgaFifoReserveSpace(bytes + sizeof type);
	cmd[0] = type;
	return cmd + 1;
}

void svgaFifoCommit(uint32_t bytes)
{
	uint32_t nextCmd = device.fifo.mapped[SVGA_FIFO_NEXT_CMD];
	uint32_t max = device.fifo.mapped[SVGA_FIFO_MAX];
	uint32_t min = device.fifo.mapped[SVGA_FIFO_MIN];

	device.fifo.reservedSize = 0;

	nextCmd += bytes;
	if(nextCmd >= max)
	{
		nextCmd -= max - min;
	}
	device.fifo.mapped[SVGA_FIFO_NEXT_CMD] = nextCmd;
	device.fifo.mapped[SVGA_FIFO_RESERVED] = 0;
}

bool svgaFifoHasCapability(int cap)
{
	return (device.fifo.mapped[SVGA_FIFO_CAPABILITIES] & cap) != 0;
}


uint32_t svgaReadReg(uint32_t index)
{
	g_io_port_write_dword(device.ioBase + SVGA_INDEX_PORT, index);
	return g_io_port_read_dword(device.ioBase + SVGA_VALUE_PORT);
}

void svgaWriteReg(uint32_t index, uint32_t value)
{
	g_io_port_write_dword(device.ioBase + SVGA_INDEX_PORT, index);
	g_io_port_write_dword(device.ioBase + SVGA_VALUE_PORT, value);
}
