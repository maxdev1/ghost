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

#include "ghost/syscall.h"
#include "ghost/system.h"
#include "ghost/system/callstructs.h"

/**
 *
 */
void g_get_efi_framebuffer(g_address* outFramebuffer, uint16_t* outWidth, uint16_t* outHeight, uint16_t* outBpp, uint32_t* outPitch)
{
	g_syscall_get_efi_framebuffer data;

	g_syscall(G_SYSCALL_GET_EFI_FRAMEBUFFER, (g_address) &data);

	*outFramebuffer = data.address;
	*outWidth = data.width;
	*outHeight = data.height;
	*outBpp = data.bpp;
	*outPitch = data.pitch;
}
