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

#include "ghost/user.h"

uint32_t processorReadCS()
{
    uint32_t cs;
    asm volatile("mov %%cs, %0"
                 : "=r"(cs));
    return cs;
}

/**
 * Performs a syscall. When the current task is operating in kernel mode (for
 * example while processing an interrupt handler) we are directly calling the
 * kernel entry.
 */
void g_syscall(uint32_t call, uint32_t data)
{
    if(processorReadCS() == 0x08)
    {
        g_current_process_info->kernelSyscallEntry(call, (void*) data);
    }
    else
    {
        asm volatile("int $0x80"
                     :
                     : "a"(call), "b"(data)
                     : "cc", "memory");
    }
}
