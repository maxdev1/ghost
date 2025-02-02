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

#include "vmsvgadriver.hpp"

#include <libpci/driver.hpp>
#include <ghost.h>
#include <cstdio>

svga_device_t device;

bool svgaInitializeDevice();
bool svgaIdentifyVersion();
uint32_t svgaReadReg(uint32_t index);
void svgaWriteReg(uint32_t index, uint32_t value);

int main()
{
	klog("vmsvgadriver");
	uint32_t tid = g_get_tid();
	if(!g_task_register_id(G_VMSVGA_DRIVER_IDENTIFIER))
	{
		klog("vmsvgadriver: could not register with task identifier '%s'", (char*) G_VMSVGA_DRIVER_IDENTIFIER);
		return -1;
	}

	if(!svgaInitializeDevice())
	{
		klog("Failed to properly initialize SVGA device");
		return -1;
	}

	// Set a video mode
	svgaWriteReg(SVGA_REG_CONFIG_DONE, true);
	svgaWriteReg(SVGA_REG_WIDTH, 1920);
	svgaWriteReg(SVGA_REG_HEIGHT, 1080);
	svgaWriteReg(SVGA_REG_BITS_PER_PIXEL, 32);
	svgaWriteReg(SVGA_REG_ENABLE, true);
	// uint32_t pitch = svgaReadReg(SVGA_REG_BYTES_PER_LINE);

	vmsvgaDriverReceiveMessages();
	return 0;
}

void vmsvgaDriverReceiveMessages()
{
	for(;;)
	{
		// TODO
		g_sleep(100000);
	}
}

bool svgaInitializeDevice()
{
	g_pci_identify_vmsvga_controller_response response{};
	while(true)
	{
		// TODO: Implement something that allows a task to wait until a task with a specific identifier registers
		if(pciDriverIdentifyVmSvgaController(&response))
		{
			break;
		}
		g_sleep(500);
	}
	if(!response.found)
	{
		klog("No VMSVGA controller present");
		return false;
	}

	device.ioBase = response.ioBase;
	device.fbPhysical = response.fbBase;
	device.fifoPhysical = response.fifoBase;

	if(!svgaIdentifyVersion())
	{
		klog("Failed to identify SVGA version");
		return false;
	}
	klog("SVGA device version is: %x", device.versionId);

	device.vramSize = svgaReadReg(SVGA_REG_VRAM_SIZE);
	device.fbSize = svgaReadReg(SVGA_REG_FB_SIZE);
	device.fifoSize = svgaReadReg(SVGA_REG_MEM_SIZE);
	klog("VRAM size: %x, FB size: %x, FIFO size: %x", device.vramSize, device.fbSize, device.fifoSize);

	device.fifo = (uint32_t*) g_map_mmio((void*) device.fifoPhysical, device.fifoSize);
	klog("Fifo mapped at: %x", device.fifo);
	device.fifo[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
	device.fifo[SVGA_FIFO_MAX] = device.fifoSize;
	device.fifo[SVGA_FIFO_NEXT_CMD] = device.fifo[SVGA_FIFO_MIN];
	device.fifo[SVGA_FIFO_STOP] = device.fifo[SVGA_FIFO_MIN];
	svgaWriteReg(SVGA_REG_ENABLE, true);
	svgaWriteReg(SVGA_REG_CONFIG_DONE, true);
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
