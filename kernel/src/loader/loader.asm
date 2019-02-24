;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;*                                                                           *
;*  Ghost, a micro-kernel based operating system for the x86 architecture    *
;*  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
;*                                                                           *
;*  This program is free software: you can redistribute it and/or modify     *
;*  it under the terms of the GNU General Public License as published by     *
;*  the Free Software Foundation, either version 3 of the License, or        *
;*  (at your option) any later version.                                      *
;*                                                                           *
;*  This program is distributed in the hope that it will be useful,          *
;*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
;*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
;*  GNU General Public License for more details.                             *
;*                                                                           *
;*  You should have received a copy of the GNU General Public License        *
;*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
;*                                                                           *
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


BITS 32

; # MULTIBOOT
section .multiboot
	MODULEALIGN		equ		1 << 0
	MEMINFO			equ 	1 << 1
	FLAGS			equ		MODULEALIGN | MEMINFO
	MAGIC			equ		0x1BADB002
	CHECKSUM		equ		-(MAGIC + FLAGS)

	; Fill constants into memory (dwords)
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM



; # CODE
section .text

	; Entry point for GRUB
	global loaderEntry

	; Initialization method
	extern loaderMain

	; Create the initial loader stack
	STACKSIZE equ 0x1000

	; Calls the initialization routines
	loaderEntry:
		; Set the stack
		mov esp, stack + STACKSIZE
		mov ebp, esp

		; We don't want interrupts until the kernel is ready
		cli

		; Call the loader
		push eax ; Magic number
		push ebx ; Multiboot information pointer
	    call loaderMain

		; Hang the system after execution
		cli
		hlt


; # DATA
section .bss

	; Align the location of the following res-commands
	align 4

	; Reserves space for the initial kernel stack
	stack:
		resb STACKSIZE


