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

#include "vbedriver.hpp"

#include <ghost.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

int main()
{
	uint32_t tid = g_get_tid();
	if (!g_task_register_id(G_VBE_DRIVER_IDENTIFIER))
	{
		klog("vbedriver: could not register with task identifier '%s'", (char *)G_VBE_DRIVER_IDENTIFIER);
		return -1;
	}

	vbeReceiveMessages();
	return 0;
}

void vbeReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + sizeof(g_vbe_set_mode_request) /*TODO*/;
	uint8_t buf[buflen];

	for (;;)
	{
		auto status = g_receive_message(buf, buflen);
		if (status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			continue;
		}

		g_message_header *header = (g_message_header *)buf;
		g_vbe_request_header *request = (g_vbe_request_header *)G_MESSAGE_CONTENT(buf);

		if (request->command == G_VBE_COMMAND_SET_MODE)
		{
			vbeHandleCommandSetMode((g_vbe_set_mode_request *)request, header->sender, header->transaction);
		}
		else
		{
			klog("vbedriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}

void vbeHandleCommandSetMode(g_vbe_set_mode_request *request, g_tid requestingTaskId, g_message_transaction requestTransaction)
{
	// create response
	g_vbe_set_mode_response response;

	// switch video mode
	g_vbe_vesa_video_info result;
	uint16_t resX = request->width;
	uint16_t resY = request->height;
	uint8_t bpp = request->bpp;

	klog("vbedriver: attempting to set video mode to %ix%i@%i", resX, resY, bpp);
	if (vbeSetVideoMode(resX, resY, bpp, result))
	{
		uint32_t lfbSize = result.bytesPerScanline * result.resolutionY;
		void *addressInRequestersSpace = g_share_mem(result.lfb, lfbSize, requestingTaskId);

		response.status = G_VBE_SET_MODE_STATUS_SUCCESS;
		response.mode_info.lfb = (uint32_t)addressInRequestersSpace;
		response.mode_info.resX = result.resolutionX;
		response.mode_info.resY = result.resolutionY;
		response.mode_info.bpp = (uint8_t)result.bpp;
		response.mode_info.bpsl = (uint16_t)result.bytesPerScanline;
	}
	else
	{
		klog("vbedriver: unable to switch to video resolution %ix%i@%i", resX, resY, bpp);
		response.status = G_VBE_SET_MODE_STATUS_FAILED;
	}

	// send response
	g_send_message_t(requestingTaskId, &response, sizeof(g_vbe_set_mode_response), requestTransaction);
}

bool vbeSetVideoMode(uint16_t width, uint16_t height, uint8_t bpp, g_vbe_vesa_video_info &result)
{
	bool success = false;
	g_vbe_info_block *vbeInfoBlock = (g_vbe_info_block *)g_lower_malloc(
		VBE_INFO_BLOCK_SIZE);

	if (vm86LoadVbeInfo(vbeInfoBlock))
	{
		klog("vbedriver: version %x", (uint32_t)vbeInfoBlock->version);

		g_vbe_mode_info_block *modeInfoBlock = (g_vbe_mode_info_block *)g_lower_malloc(VBE_MODE_INFO_BLOCK_SIZE);
		uint32_t mode = vbeFindBestMatchingVideoMode(vbeInfoBlock, modeInfoBlock, width, height, bpp);
		if (mode)
		{
			if (vbeApplyVideoMode(mode, modeInfoBlock, result))
			{
				success = true;
			}
		}
		g_lower_free(modeInfoBlock);
	}
	else
	{
		klog("vbedriver: failed to load basic VBE information");
	}

	g_lower_free(vbeInfoBlock);

	return success;
}

uint32_t vbeFindBestMatchingVideoMode(g_vbe_info_block *vbeInfoBlock, g_vbe_mode_info_block *modeInfoBlock,
									  uint16_t width, uint16_t height, uint8_t bpp)
{
	uint32_t bestMatchingMode = 0;
	uint32_t bestFoundDepthDiff = -1;
	uint32_t bestFoundResolutionDiff = -1;
	uint32_t wantedResolution = width * height;

	uint16_t *modes = (uint16_t *)G_FP_TO_LINEAR(vbeInfoBlock->videoModeFarPtr);
	for (uint32_t i = 0;; ++i)
	{
		uint16_t mode = modes[i];
		if (mode == 0xFFFF)
		{
			break;
		}

		if (!vm86LoadModeInfo(mode, modeInfoBlock))
		{
			klog("vbedriver: mode %i: could not load mode info block, skipping", mode);
			continue;
		}

		// Must be supported by hardware
		if ((modeInfoBlock->modeAttributes & 0x1) != 0x1)
		{
			continue;
		}

		// Need LFB support
		if ((modeInfoBlock->modeAttributes & 0x90) != 0x90)
		{
			continue;
		}

		// Need direct color mode
		if (modeInfoBlock->memoryModel != 6)
		{
			continue;
		}

		// Check if it's matching better
		uint32_t resolution = modeInfoBlock->resolutionX * modeInfoBlock->resolutionY;
		uint32_t resolutionDiff = (resolution > wantedResolution) ? (resolution - wantedResolution) : (wantedResolution - resolution);
		uint32_t depthDiff = (modeInfoBlock->bpp > bpp) ? (modeInfoBlock->bpp - bpp) : (bpp - modeInfoBlock->bpp);

		if (resolutionDiff < bestFoundResolutionDiff || (resolutionDiff == bestFoundResolutionDiff && depthDiff < bestFoundDepthDiff))
		{
			bestMatchingMode = mode;
			bestFoundDepthDiff = depthDiff;
			bestFoundResolutionDiff = resolutionDiff;

			// Break on perfect match
			if (depthDiff == 0 && resolutionDiff == 0)
			{
				break;
			}
		}
	}

	return bestMatchingMode;
}

bool vbeApplyVideoMode(uint32_t mode, g_vbe_mode_info_block *modeInfoBlock, g_vbe_vesa_video_info &result)
{
	klog("vbedriver: switching to video mode %i", mode);

	if (vm86SwitchVideoMode(mode) && vm86LoadModeInfo(mode, modeInfoBlock))
	{
		void *area = g_map_mmio((void *)modeInfoBlock->lfbPhysicalBase, modeInfoBlock->linBytesPerScanline * modeInfoBlock->resolutionY);
		result.resolutionX = modeInfoBlock->resolutionX;
		result.resolutionY = modeInfoBlock->resolutionY;
		result.bpp = modeInfoBlock->bpp;
		result.bytesPerScanline = modeInfoBlock->linBytesPerScanline;
		result.lfb = area;
		return true;
	}
	return false;
}

bool vm86LoadVbeInfo(g_vbe_info_block *target)
{
	g_vm86_registers out;
	g_vm86_registers in;

	g_far_pointer vbeInfoBlockFp = G_LINEAR_TO_FP((uint32_t)target);

	in.ax = 0x4F00;
	in.bx = 0;
	in.cx = 0;
	in.dx = 0;
	in.es = G_FP_SEG(vbeInfoBlockFp);
	in.di = G_FP_OFF(vbeInfoBlockFp);
	in.ds = 0;
	in.si = 0;

	g_call_vm86(0x10, &in, &out);

	return (out.ax == 0x4F);
}

bool vm86LoadModeInfo(uint16_t mode, g_vbe_mode_info_block *target)
{
	g_vm86_registers out;
	g_vm86_registers regs;

	g_far_pointer modeInfoBlockFp = G_LINEAR_TO_FP((uint32_t)target);

	regs.ax = 0x4F01;
	regs.cx = mode;
	regs.es = G_FP_SEG(modeInfoBlockFp);
	regs.di = G_FP_OFF(modeInfoBlockFp);

	g_call_vm86(0x10, &regs, &out);

	return (out.ax == 0x4F);
}

bool vm86SwitchVideoMode(uint32_t mode)
{
	g_vm86_registers out;
	g_vm86_registers regs;

	regs.ax = 0x4F02;
	regs.bx = mode;

	regs.bx |= 0x4000; // Flat frame buffer

	g_call_vm86(0x10, &regs, &out);

	return (out.ax == 0x4F);
}
