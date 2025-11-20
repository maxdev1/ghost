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

#ifndef GHOST_API_SYSTEM_CALLSTRUCTS
#define GHOST_API_SYSTEM_CALLSTRUCTS

#include "../common.h"
#include "../stdint.h"
#include "types.h"

__BEGIN_C

/**
 * @field interrupt
 * 		the interrupt to call
 *
 * @field in
 * 		the input registers
 *
 * @field out
 * 		the output registers
 *
 * @field status
 * 		status of the call
 */
typedef struct
{
	uint32_t interrupt;
	g_vm86_registers in;
	g_vm86_registers* out;

	g_vm86_call_status status;
}__attribute__((packed)) g_syscall_call_vm86;


/**
 * @field message
 * 		the message
 */
typedef struct
{
	char* message;
}__attribute__((packed)) g_syscall_log;

/**
 * @field fd
 * 		the opened fd
 */
typedef struct
{
	g_fd fd;
}__attribute__((packed)) g_syscall_open_log_pipe;

/**
 * @field enabled
 * 		whether or not to enable the video log
 */
typedef struct
{
	uint8_t enabled;
}__attribute__((packed)) g_syscall_set_video_log;

/**
 * @field test
 * 		test value
 *
 * @field result
 * 		test result
 */
typedef struct
{
	uint32_t test;

	uint32_t result;
}__attribute__((packed)) g_syscall_test;

/**
 *
 */
typedef struct
{
	uint32_t source;
	uint32_t irq;
} __attribute__((packed)) g_syscall_irq_create_redirect;

/**
 * @field irq
 * 		irq to wait for
 * @field timeout
 *      timeout in milliseconds
 */
typedef struct
{
	uint8_t irq;
	uint32_t timeout;
} __attribute__((packed)) g_syscall_await_irq;

/**
 * @field address
 *		framebuffer address
 * @field width
*		framebuffer width
 * @field height
*		framebuffer height
 * @field bpp
*		framebuffer bpp
 * @field pitch
 *		framebuffer pitch
 */
typedef struct
{
	g_address address;
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
	uint32_t pitch;
} __attribute__((packed)) g_syscall_get_efi_framebuffer;

__END_C

#endif
