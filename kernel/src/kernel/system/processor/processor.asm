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
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


BITS 64

global _enableSSE

;
; void enableSSE()
;
; Prepares the necessary CPU flags to enable SSE instructions
_enableSSE:
    clts
    fninit
    fclex

    push qword 0x037F
    fldcw [rsp]         ; load default control word
    add rsp, 8

    mov rax, cr0
    and rax, ~(1 << 2)  ; clear CR0.EM coprocessor emulation
    or rax, (1 << 1)    ; set CR0.MP monitor coprocessor
    mov cr0, rax

    mov rax, cr4
    or rax, (1 << 9)    ; set CR4.OSFXSR
    or rax, (1 << 10)   ; set CR4.OSXMMEXCPT
    mov cr4, rax

    push qword 0x1F80
    ldmxcsr [rsp]       ; load default settings to MXCSR
    add rsp, 8

    ret

