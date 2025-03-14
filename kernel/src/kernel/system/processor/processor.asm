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

global _checkForCPUID
global _enableSSE

;
; bool checkForCPUID()
;
; Checks if the CPUID instruction is available. This is implemented in
; pure assembly, because C/C++ could cause race conditions due to the
; ability of the compiler to modify the EFLAGS when it needs to.
;
_checkForCPUID:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 0x200000
	push eax
	popfd
	pushfd
	pop eax
	xor eax, ecx
	shr eax, 21
	and eax, 1
	push ecx
	popfd
	ret

;
; void enableSSE()
;
; Prepares the necessary CPU flags to enable SSE instructions
_enableSSE:
    clts
    fninit
    fclex

    push dword 0x037F
    fldcw [esp]         ; load default control word
    add esp, 4

    mov eax, cr0
    and eax, ~(1 << 2)  ; clear CR0.EM coprocessor emulation
    or eax, (1 << 1)    ; set CR0.MP monitor coprocessor
    mov cr0, eax

    mov eax, cr4
    or eax, (1 << 9)    ; set CR4.OSFXSR
    or eax, (1 << 10)   ; set CR4.OSXMMEXCPT
    mov cr4, eax

    push dword 0x1F80
    ldmxcsr [esp]       ; load default settings to MXCSR
    add esp, 4

	ret

