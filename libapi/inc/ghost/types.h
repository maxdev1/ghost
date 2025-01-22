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

#ifndef __GHOST_SYS_TYPES__
#define __GHOST_SYS_TYPES__

#include "ghost/common.h"
#include "ghost/stdint.h"

__BEGIN_C

// address types
#if __i386__
typedef uint32_t g_address;
#define G_ADDRESS_MAX      0xFFFFFFFFl
#elif __x86_64__
typedef uint64_t g_address;
#define G_ADDRESS_MAX      0xFFFFFFFFFFFFFFFFl
#endif
typedef g_address g_physical_address;
typedef g_address g_virtual_address;
typedef g_address g_offset;
typedef uint32_t g_far_pointer;
typedef g_address g_ptrsize;
typedef g_address g_size;

// extracts parts from far pointers
#define G_FP_SEG(fp)        			(((g_far_pointer) fp) >> 16)
#define G_FP_OFF(fp)        			(((g_far_pointer) fp) & 0xFFFF)

// far pointer conversion macros
#define G_SEGOFF_TO_LINEAR(seg, off)	((void*) (((seg & 0xFFFF) << 4) + (off & 0xFFFF)))
#define G_FP_TO_LINEAR(fp)				G_SEGOFF_TO_LINEAR(G_FP_SEG(fp), G_FP_OFF(fp))

#define G_SEGOFF_TO_FP(seg, off)		((g_far_pointer) (((seg & 0xFFFF) << 16) | (off & 0xFFFF)))
#define G_LINEAR_TO_FP(linear)			((linear > 0x100000) ? 0 : ((((linear >> 4) & 0xFFFF) << 16) + (linear & 0xFL)))

// type used for user mutexes
typedef uint32_t g_user_mutex;
typedef uint8_t g_bool;

__END_C

#endif
