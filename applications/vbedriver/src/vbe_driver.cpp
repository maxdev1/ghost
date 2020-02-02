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

#include <vbe_driver.hpp>
#include <ghostuser/graphics/vbe.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghost.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/**
 *
 */
bool loadVbeInfo(VbeInfoBlock* target) {

	g_vm86_registers out;
	g_vm86_registers in;

	g_far_pointer vbeInfoBlockFp = G_LINEAR_TO_FP((uint32_t ) target);

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

/**
 *
 */
bool loadModeInfo(uint16_t mode, ModeInfoBlock* target) {

	g_vm86_registers out;
	g_vm86_registers regs;

	g_far_pointer modeInfoBlockFp = G_LINEAR_TO_FP((uint32_t ) target);

	regs.ax = 0x4F01;
	regs.cx = mode;
	regs.es = G_FP_SEG(modeInfoBlockFp);
	regs.di = G_FP_OFF(modeInfoBlockFp);

	g_call_vm86(0x10, &regs, &out);

	return (out.ax == 0x4F);
}

/**
 *
 */
bool setVideoMode(uint32_t mode, bool flatFrameBuffer) {
	g_vm86_registers out;
	g_vm86_registers regs;

	regs.ax = 0x4F02;
	regs.bx = mode;

	if (flatFrameBuffer) {
		regs.bx |= 0x4000;
	}

	g_call_vm86(0x10, &regs, &out);

	return (out.ax == 0x4F);
}

/**
 *
 */
bool setVideoMode(uint32_t wantedWidth, uint32_t wantedHeight, uint32_t wantedBpp, VesaVideoInfo& result) {

	bool successful = false;
	bool debug_output = false;

	// Get VBE mode info
	VbeInfoBlock* vbeInfoBlock = (VbeInfoBlock*) g_lower_malloc(
	VBE_INFO_BLOCK_SIZE);

	g_logger::log("loading VBE info block");
	bool couldLoadVbeInfo = loadVbeInfo(vbeInfoBlock);
	if (!couldLoadVbeInfo) {
		g_logger::log("could not load VBE info block");
	} else {
		g_logger::log("loaded vbe info, version %x", (uint32_t) vbeInfoBlock->version);

		// Load modes
		ModeInfoBlock* modeInfoBlock = (ModeInfoBlock*) g_lower_malloc(
		VBE_MODE_INFO_BLOCK_SIZE);

		uint32_t bestMatchingMode = 0;
		uint32_t bestFoundDepthDiff = -1;
		uint32_t bestFoundResolutionDiff = -1;
		uint32_t wantedResolution = wantedWidth * wantedHeight;

		g_logger::log("farptr: %x, seg: %x, off: %x, linear: %x", vbeInfoBlock->videoModeFarPtr, G_FP_SEG(vbeInfoBlock->videoModeFarPtr),
				G_FP_OFF(vbeInfoBlock->videoModeFarPtr), G_FP_TO_LINEAR(vbeInfoBlock->videoModeFarPtr));
		uint16_t* modes = (uint16_t*) G_FP_TO_LINEAR(vbeInfoBlock->videoModeFarPtr);
		for (uint32_t i = 0;; ++i) {
			uint16_t mode = modes[i];

			if (mode == 0xFFFF) {
				break;
			}

			bool couldLoadModeInfo = loadModeInfo(mode, modeInfoBlock);

			if (!couldLoadModeInfo) {
				g_logger::log("mode %x: could not load mode info block, skipping", mode);
				continue;
			}

			// mode output
			if (debug_output) {
				g_logger::log("mode %x, attr: %x, mmo: %x, %ix%ix%i lfb: %x", mode, modeInfoBlock->modeAttributes, modeInfoBlock->memoryModel,
						modeInfoBlock->resolutionX, modeInfoBlock->resolutionY, modeInfoBlock->bpp, modeInfoBlock->lfbPhysicalBase);
			}

			// Must be supported by hardware
			if ((modeInfoBlock->modeAttributes & 0x1) != 0x1) {
				continue;
			}

			// Need LFB support
			if ((modeInfoBlock->modeAttributes & 0x90) != 0x90) {
				continue;
			}

			// Need direct color mode
			if (modeInfoBlock->memoryModel != 6) {
				continue;
			}

			// Check if it's matching better
			uint32_t resolution = modeInfoBlock->resolutionX * modeInfoBlock->resolutionY;
			uint32_t resolutionDiff = (resolution > wantedResolution) ? (resolution - wantedResolution) : (wantedResolution - resolution);
			uint32_t depthDiff = (modeInfoBlock->bpp > wantedBpp) ? (modeInfoBlock->bpp - wantedBpp) : (wantedBpp - modeInfoBlock->bpp);

			if (resolutionDiff < bestFoundResolutionDiff || (resolutionDiff == bestFoundResolutionDiff && depthDiff < bestFoundDepthDiff)) {
				bestMatchingMode = mode;
				bestFoundDepthDiff = depthDiff;
				bestFoundResolutionDiff = resolutionDiff;

				// g_logger::log("vbe: updated best matching mode to %ix%ix%i", modeInfoBlock->resolutionX, modeInfoBlock->resolutionY, modeInfoBlock->bpp);

				// Break on perfect match
				if (depthDiff == 0 && resolutionDiff == 0) {
					break;
				}
			}
		}

		// If a matching mode was found
		if (bestMatchingMode != 0) {

			// Enable the best matching mode
			klog("performing mode switch to %x...", bestMatchingMode);
			if (setVideoMode(bestMatchingMode, true)) {

				// Reload mode info into buffer
				bool couldReloadModeInfo = loadModeInfo(bestMatchingMode, modeInfoBlock);

				// Reloading successful?
				if (couldReloadModeInfo) {

					// Create MMIO mapping
					void* area = g_map_mmio((void*) modeInfoBlock->lfbPhysicalBase, modeInfoBlock->linBytesPerScanline * modeInfoBlock->resolutionY);

					// Write out
					result.resolutionX = modeInfoBlock->resolutionX;
					result.resolutionY = modeInfoBlock->resolutionY;
					result.bpp = modeInfoBlock->bpp;
					result.bytesPerScanline = modeInfoBlock->linBytesPerScanline;
					result.lfb = area;
					successful = true;
				}
			}
		}

		g_lower_free(modeInfoBlock);
	}

	g_lower_free(vbeInfoBlock);

	return successful;
}

/**
 *
 */
int main() {

	uint32_t tid = g_get_tid();
	if (!g_task_register_id(G_VBE_DRIVER_IDENTIFIER)) {
		g_logger::log("vbedriver: could not register with task identifier '%s'", (char*) G_VBE_DRIVER_IDENTIFIER);
		return -1;
	}

	g_logger::log("initialized");

	size_t buflen = sizeof(g_message_header) + sizeof(g_vbe_set_mode_request);
	uint8_t buf[buflen];

	while (true) {
		// wait for incoming request
		auto status = g_receive_message(buf, buflen);
		if (status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			continue;
		}

		g_message_header* header = (g_message_header*) buf;
		g_vbe_request_header* vbeheader = (g_vbe_request_header*) G_MESSAGE_CONTENT(buf);
		uint32_t requester = header->sender;

		// handle command
		if (vbeheader->command == G_VBE_COMMAND_SET_MODE) {
			klog("vbe driver received: setmode");
			g_vbe_set_mode_request* request = (g_vbe_set_mode_request*) G_MESSAGE_CONTENT(buf);

			// create response
			g_vbe_set_mode_response response;

			// switch video mode
			VesaVideoInfo result;
			uint16_t resX = request->width;
			uint16_t resY = request->height;
			uint8_t bpp = request->bpp;

			klog("attempting to set video mode");
			if (setVideoMode(resX, resY, bpp, result)) {
				g_logger::log("changed video mode to %ix%ix%i", resX, resY, bpp);
				uint32_t lfbSize = result.bytesPerScanline * result.resolutionY;
				void* addressInRequestersSpace = g_share_mem(result.lfb, lfbSize, requester);

				response.status = G_VBE_SET_MODE_STATUS_SUCCESS;
				response.mode_info.lfb = (uint32_t) addressInRequestersSpace;
				response.mode_info.resX = result.resolutionX;
				response.mode_info.resY = result.resolutionY;
				response.mode_info.bpp = (uint8_t) result.bpp;
				response.mode_info.bpsl = (uint16_t) result.bytesPerScanline;

			} else {
				g_logger::log("unable to switch to video resolution %ix%ix%i", resX, resY, bpp);
				response.status = G_VBE_SET_MODE_STATUS_FAILED;
			}

			// send response
			g_send_message_t(header->sender, &response, sizeof(g_vbe_set_mode_response), header->transaction);
		} else {
			g_logger::log("received unknown command %i from task %i", vbeheader->command, header->sender);
		}
	}

	return 0;
}

