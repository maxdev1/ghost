;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;*                                                                           *
;*  Ghost, a micro-kernel based operating system for the x86_64 architecture *
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
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


; ############################################################################
;
;                                  NOTE
;
;   At this point, no stack is available, don't use PUSH or POP!
;
;   This file is compiled to the ramdisk and loaded by the SMP mechanism to
;   start up APs.
;
;   The kernel prepares the three values for page directory, stack location
;   and startup location.
;
; ############################################################################


; Loaded by the kernel to this location
org 0x1000

; Initial setup
BITS 16
startup:
	; Load the GDT
    lgdt [gdtPointer]

    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

	; Far-JMP into protected mode code
    jmp 0x08:protectedStart


; Protected mode for 64-bit (transition to 64-bit long mode)
BITS 64
protectedStart:
	; Set up code segments for 64-bit mode
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

	; Lock all cores
	acquireLock:
    lock bts qword [interlock], 0
    jc acquireLock

	    ; Set page directory
	    mov rax, [0x500]
	    mov cr3, rax

		; Enable paging
	    mov rax, cr0
	    or rax, 0x80000000
	    mov cr0, rax

	    ; Set "global pages" flag
	    mov rax, cr4
	    or rax, 0x80
	    mov cr4, rax

		; Load stack from stack array
		mov rax, [0x508]		; current index
		shl rax, 2				; multiply by 4 (size of one entry)
		mov rsp, [0x50C + rax]	; move entry to RSP
		mov rbp, rsp

		; Increment AP counter
		inc qword [0x508]

	; Release lock
    lock btr qword [interlock], 0

	; Jump to kernel
	call [0x504]

	; AP should never exit, just for safety
	hang:
	hlt
	jmp hang


; Inter-core synchronization
interlock:
	dd 0x00000000


; Pointer to the GDT
gdtPointer:
	dw 24
	dd gdt

; Basic setup GDT for x86_64
gdt:
	; null descriptor
	dq 0x0000000000000000

	; code descriptor (64-bit)
	dw 0xFFFF
	dw 0x0000
	dw 0x9A00
	dw 0x00CF
	dq 0x0000000000000000

	; data descriptor (64-bit)
	dw 0xFFFF
	dw 0x0000
	dw 0x9200
	dw 0x00CF
	dq 0x0000000000000000
