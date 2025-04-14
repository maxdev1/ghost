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

; Real mode
BITS 16
startup:
	; Load the GDT
    lgdt [gdtPointer]

    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

	; Far-JMP into protected mode code
    jmp 0x8:protectedStart


; Protected mode
BITS 32
protectedStart:
	; Code segments were set by far JMP, set data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

	; Lock all cores
    acquireLock:
    lock bts dword [interlock], 0
    jc acquireLock

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set page directory
    mov eax, [0x500]
    mov cr3, eax

    ; Enable PGE
    mov eax, cr4
    or eax, 1 << 7
    mov cr4, eax

    ; Set up EFER MSR to enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Get index of this AP and increment counter
    mov eax, [0x510]
    inc dword [0x510]

	; Release lock
    lock btr dword [interlock], 0

	; Far-jump to 64 bits
	jmp 0x18:longStart

; Long mode
[BITS 64]
longStart:
    ; Set data segments
    mov bx, 0x10
    mov ds, bx
    mov ss, bx

    ; 32-bit code provides AP index in EAX
    ; From the stack array, take the entry at offset multiplied by 8
    shl rax, 3
    mov rsp, [0x518 + rax]
    mov rbp, rsp

	; Jump into kernel code
    mov rax, [0x508]
    jmp rax

; Inter-core synchronization
interlock:
	dd 0


; Pointer to the GDT
gdtPointer:
	dw 31
	dd gdt

; Basic setup GDT
gdt:
	; null descriptor
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dw 0x0000

	; code descriptor
	dw 0xFFFF
	dw 0x0000
	dw 0x9800
	dw 0x00CF

	; data descriptor
	dw 0xFFFF
	dw 0x0000
	dw 0x9200
	dw 0x00CF

	; code descriptor (64-bit)
	dw 0xFFFF
	dw 0x0000
	dw 0x9A00
	dw 0x00A0

    ; TODO 64 bit data segment?