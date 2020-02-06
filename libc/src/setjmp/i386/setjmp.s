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

.global __setjmp
.global _setjmp
.global setjmp
.type __setjmp,@function
.type _setjmp,@function
.type setjmp,@function

#
# int setjmp(jmp_buf env)
#
__setjmp:
_setjmp:
setjmp:
	mov	4(%esp), %eax
	mov %ebx, (%eax)
	mov %esi, 4(%eax)
	mov %edi, 8(%eax)
	mov %ebp, 12(%eax)
	lea 4(%esp), %ecx
	mov %ecx, 16(%eax)
	mov (%esp), %ecx
	mov %ecx, 20(%eax)
	xor %eax, %eax
	ret
