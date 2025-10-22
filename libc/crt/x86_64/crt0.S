#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                           *
#*  Ghost, a micro-kernel based operating system for the x86 architecture    *
#*  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
#*                                                                           *
#*  This program is free software: you can redistribute it and/or modify     *
#*  it under the terms of the GNU General Public License as published by     *
#*  the Free Software Foundation, either version 3 of the License, or        *
#*  (at your option) any later version.                                      *
#*                                                                           *
#*  This program is distributed in the hope that it will be useful,          *
#*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
#*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
#*  GNU General Public License for more details.                             *
#*                                                                           *
#*  You should have received a copy of the GNU General Public License        *
#*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
#*                                                                           *
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

# Symbol must be visible to the outside
.global _start

# <__g_main> and <g_exit> are provided by the library
.extern __g_main
.extern g_exit

# The <_start> function is where the kernel starts the execution.
_start:
    xorq %rbp, %rbp
    andq $-16, %rsp
    call __g_main

# Endless loop, for the case that bad things happen
wait:
	hlt
	jmp wait
