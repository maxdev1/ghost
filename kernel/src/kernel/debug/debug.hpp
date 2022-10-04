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

#ifndef __KERNEL_DEBUG__
#define __KERNEL_DEBUG__

#define DEBUG_TRACE_STACK                                  \
	{                                                      \
		uint32_t ebpv;                                     \
		asm volatile("push %%ebp\n"                        \
					 "pop %0"                              \
					 : "=g"(ebpv));                        \
		uint32_t* ebp = reinterpret_cast<uint32_t*>(ebpv); \
		logInfo("%# stack trace:");                        \
		for(int frame = 0; frame < 8; ++frame)             \
		{                                                  \
			uint32_t eip = ebp[1];                         \
			if(eip == 0)                                   \
				break;                                     \
			ebp = reinterpret_cast<uint32_t*>(ebp[0]);     \
			logInfo("%#  %h", eip);                        \
		}                                                  \
	}

#endif
