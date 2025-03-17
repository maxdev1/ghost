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

#ifndef __VBEDRIVER__
#define __VBEDRIVER__

#include <libvideo/videodriver.hpp>

#include <cstdint>
#include <ghost.h>

/**
 *
 */
#define VBE_INFO_BLOCK_SIZE 512
struct g_vbe_info_block
{
	// Signature should be "VESA" (0x56455341)
	uint8_t signature[4];
	// For example 0x300 for VESA 3.0
	uint16_t version;

	// OEM string pointer
	g_far_pointer oemStringFarPtr;
	// Capability information
	uint8_t capabilities[4];
	// Video modes
	g_far_pointer videoModeFarPtr;

	// Size of video memory in 64KiB blocks
	uint16_t memoryBlockCount;

	// OEM information
	uint16_t oemSoftwareRevision;
	g_far_pointer oemVendorNameStringFarPtr;
	g_far_pointer oemProductNameStringFarPtr;
	g_far_pointer oemProductRevisionFarPtr;
} __attribute__((packed));

/**
 *
 */
#define VBE_MODE_INFO_BLOCK_SIZE 256
struct g_vbe_mode_info_block
{
	// 0
	uint16_t modeAttributes;
	uint8_t windowAttributesA;
	uint8_t windowAttributesB;
	uint16_t granularityKb;
	uint16_t windowSizeKb;
	uint16_t segmentA;
	uint16_t segmentB;
	g_far_pointer windowFunctionFarPtr;

	// 16
	uint16_t bytesPerScanline;
	uint16_t resolutionX;
	uint16_t resolutionY;
	uint8_t charSizeX;
	uint8_t charSizeY;
	uint8_t planes;
	uint8_t bpp;
	uint8_t banks;
	uint8_t memoryModel;
	uint8_t bankSizeKb;
	uint8_t imagePages;
	uint8_t reserved0;

	// 31
	uint8_t redMaskSize;
	uint8_t redFieldPosition;
	uint8_t greenMaskSize;
	uint8_t greenFieldPosition;
	uint8_t blueMaskSize;
	uint8_t blueFieldPosition;
	uint8_t rsvdMaskSize;
	uint8_t rsvdFieldPosition;
	uint8_t directColorModeInfo;

	// 40
	uint32_t lfbPhysicalBase;
	uint32_t offScreenMemOffset;
	uint16_t offScreenMemSizeKb;
	uint16_t linBytesPerScanline;
	uint8_t bnkNumberOfImagePages;
	uint8_t linNumberOfImagePages;
	uint8_t linRedMaskSize;
	uint8_t linRedFieldPosition;
	uint8_t linGreenMaskSize;
	uint8_t linGreenFieldPosition;
	uint8_t linBlueMaskSize;
	uint8_t linkBlueFieldPosition;
	uint8_t linReservedMaskSize;
	uint8_t linReservedFieldPosition;
	uint32_t maxPixelClock;
} __attribute__((packed));

/**
 * Info about the enabled video mode
 */
struct g_vbe_vesa_video_info
{
	uint32_t resolutionX;
	uint32_t resolutionY;
	uint8_t bpp;
	uint32_t bytesPerScanline;
	void *lfb;
};

/**
 * Main loop receiving messages from other processes to do something.
 */
void vbeReceiveMessages();

/**
 * Handles a set-mode command.
 */
void vbeHandleCommandSetMode(g_video_set_mode_request *request, g_tid requestingTaskId, g_message_transaction requestTransaction);

/**
 * Attempts to set the video mode to the specified parameters.
 */
bool vbeSetVideoMode(uint16_t width, uint16_t wantedHeight, uint8_t wantedBpp, g_vbe_vesa_video_info &result);

/**
 * Checks all existing video modes to find the best matching video mode.
 */
uint32_t vbeFindBestMatchingVideoMode(g_vbe_info_block *vbeInfoBlock, g_vbe_mode_info_block *modeInfoBlock,
									  uint16_t width, uint16_t height, uint8_t bpp);

/**
 * Switches to a given video mode.
 */
bool vbeApplyVideoMode(uint32_t mode, g_vbe_mode_info_block *modeInfoBlock, g_vbe_vesa_video_info &result);

bool vm86LoadVbeInfo(g_vbe_info_block *target);
bool vm86LoadModeInfo(uint16_t mode, g_vbe_mode_info_block *target);
bool vm86SwitchVideoMode(uint32_t mode);

#endif
