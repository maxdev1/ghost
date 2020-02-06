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

.global __longjmp
.global _longjmp
.global longjmp
.type __longjmp,@function
.type _longjmp,@function
.type longjmp,@function

#
# void longjmp(jmp_buf env, int val)
#
__longjmp:
_longjmp:
longjmp:
	mov 4(%esp), %edx
	mov 8(%esp), %eax
	test %eax, %eax
	jnz 1f
	inc %eax
1:
	mov (%edx), %ebx
	mov 4(%edx), %esi
	mov 8(%edx), %edi
	mov 12(%edx), %ebp
	mov 16(%edx), %ecx
	mov %ecx, %esp
	mov 20(%edx), %ecx
	jmp *%ecx
