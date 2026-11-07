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
#include "irq.h"
#include "../filesystem/types.h"
#include "../memory/types.h"

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

typedef uint8_t g_irq_operation;
#define G_IRQ_BIND ((g_irq_operation) 1)
#define G_IRQ_WAIT ((g_irq_operation) 2)
#define G_IRQ_COMPLETE ((g_irq_operation) 3)
#define G_IRQ_MASK ((g_irq_operation) 4)
#define G_IRQ_RELEASE ((g_irq_operation) 5)

/**
 * Fixed-width syscall payload for all common IRQ operations. Only a bind
 * supplies source and line. Only a wait supplies timeoutMs. The kernel
 * returns the status and, for a successful bind, the opaque handle.
 */
typedef struct g_syscall_irq
{
	g_irq_handle handle;
	uint32_t line;
	uint32_t timeoutMs;
	g_irq_operation operation;
	g_irq_source source;
	g_irq_status status;
} __attribute__((packed)) g_syscall_irq;

__END_C

#endif
