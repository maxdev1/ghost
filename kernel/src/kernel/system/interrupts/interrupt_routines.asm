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

BITS 64

;
; C handler functions
;
extern _interruptHandler

;
; Handler routine
;
interruptRoutine:
    ; The processor has pushed registers to the stack.
    ; We'll save the registers that are typically pushed on interrupt, including
    ; the general-purpose registers and the instruction pointer (RIP) registers.

    ; Store registers
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx
    push rax

    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Stack pointer argument
    mov rdi, rsp
    ; Call handler
    call _interruptHandler
    ; Set stack pointer from return value
    mov rsp, rax

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8

    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp
    pop rsi
    pop rdi

    ; Skip over the interrupt error code (this was pushed before the handler call)
    add rsp, 8

    ; Now we return with IRET, the processor will pop specific registers
    ; IRET in x86_64 pops RIP, CS, and EFLAGS from the stack
    iretq


; Handling routine macros
%macro handleRoutErr 2
global %1
%1:
	; error pushed by processor
	push %2 ;intr
	jmp interruptRoutine
%endmacro

%macro handleRoutine 2
global %1
%1:
	push 0	; error
	push %2	; intr
	jmp interruptRoutine
%endmacro


; Exceptions
handleRoutine _isr00, 0x00   ; Divide by zero exception
handleRoutine _isr01, 0x01   ; Debug exception
handleRoutine _isr02, 0x02   ; Non-maskable interrupt exception
handleRoutine _isr03, 0x03   ; Breakpoint exception
handleRoutine _isr04, 0x04   ; Detected overflow exception
handleRoutine _isr05, 0x05   ; Out of bounds exception
handleRoutine _isr06, 0x06   ; Invalid operation code exception
handleRoutine _isr07, 0x07   ; No coprocessor exception
handleRoutErr _isr08, 0x08   ; Double fault exception [has error code]
handleRoutine _isr09, 0x09   ; Coprocessor segment overrun exception
handleRoutErr _isr0A, 0x0A   ; Bad TSS exception [has error code]
handleRoutErr _isr0B, 0x0B   ; Segment not present exception [has error code]
handleRoutErr _isr0C, 0x0C   ; Stack fault exception [has error code]
handleRoutErr _isr0D, 0x0D   ; General protection fault exception [has error code]
handleRoutErr _isr0E, 0x0E   ; Page fault [has error code]
handleRoutine _isr0F, 0x0F   ; Unknown interrupt exception
handleRoutine _isr10, 0x10   ; Coprocessor fault exception
handleRoutine _isr11, 0x11   ; Alignment check exception
handleRoutine _isr12, 0x12   ; Machine check exception

; Reserved exceptions
handleRoutine _isr13, 0x13
handleRoutine _isr14, 0x14
handleRoutine _isr15, 0x15
handleRoutine _isr16, 0x16
handleRoutine _isr17, 0x17
handleRoutine _isr18, 0x18
handleRoutine _isr19, 0x19
handleRoutine _isr1A, 0x1A
handleRoutine _isr1B, 0x1B
handleRoutine _isr1C, 0x1C
handleRoutine _isr1D, 0x1D
handleRoutine _isr1E, 0x1E
handleRoutine _isr1F, 0x1F

; Requests
handleRoutine _isr20, 0x20
handleRoutine _isr21, 0x21
handleRoutine _isr22, 0x22
handleRoutine _isr23, 0x23
handleRoutine _isr24, 0x24
handleRoutine _isr25, 0x25
handleRoutine _isr26, 0x26
handleRoutine _isr27, 0x27
handleRoutine _isr28, 0x28
handleRoutine _isr29, 0x29
handleRoutine _isr2A, 0x2A
handleRoutine _isr2B, 0x2B
handleRoutine _isr2C, 0x2C
handleRoutine _isr2D, 0x2D
handleRoutine _isr2E, 0x2E
handleRoutine _isr2F, 0x2F
handleRoutine _isr30, 0x30
handleRoutine _isr31, 0x31
handleRoutine _isr32, 0x32
handleRoutine _isr33, 0x33
handleRoutine _isr34, 0x34
handleRoutine _isr35, 0x35
handleRoutine _isr36, 0x36
handleRoutine _isr37, 0x37
handleRoutine _isr38, 0x38
handleRoutine _isr39, 0x39
handleRoutine _isr3A, 0x3A
handleRoutine _isr3B, 0x3B
handleRoutine _isr3C, 0x3C
handleRoutine _isr3D, 0x3D
handleRoutine _isr3E, 0x3E
handleRoutine _isr3F, 0x3F
handleRoutine _isr40, 0x40
handleRoutine _isr41, 0x41
handleRoutine _isr42, 0x42
handleRoutine _isr43, 0x43
handleRoutine _isr44, 0x44
handleRoutine _isr45, 0x45
handleRoutine _isr46, 0x46
handleRoutine _isr47, 0x47
handleRoutine _isr48, 0x48
handleRoutine _isr49, 0x49
handleRoutine _isr4A, 0x4A
handleRoutine _isr4B, 0x4B
handleRoutine _isr4C, 0x4C
handleRoutine _isr4D, 0x4D
handleRoutine _isr4E, 0x4E
handleRoutine _isr4F, 0x4F
handleRoutine _isr50, 0x50
handleRoutine _isr51, 0x51
handleRoutine _isr52, 0x52
handleRoutine _isr53, 0x53
handleRoutine _isr54, 0x54
handleRoutine _isr55, 0x55
handleRoutine _isr56, 0x56
handleRoutine _isr57, 0x57
handleRoutine _isr58, 0x58
handleRoutine _isr59, 0x59
handleRoutine _isr5A, 0x5A
handleRoutine _isr5B, 0x5B
handleRoutine _isr5C, 0x5C
handleRoutine _isr5D, 0x5D
handleRoutine _isr5E, 0x5E
handleRoutine _isr5F, 0x5F
handleRoutine _isr60, 0x60
handleRoutine _isr61, 0x61
handleRoutine _isr62, 0x62
handleRoutine _isr63, 0x63
handleRoutine _isr64, 0x64
handleRoutine _isr65, 0x65
handleRoutine _isr66, 0x66
handleRoutine _isr67, 0x67
handleRoutine _isr68, 0x68
handleRoutine _isr69, 0x69
handleRoutine _isr6A, 0x6A
handleRoutine _isr6B, 0x6B
handleRoutine _isr6C, 0x6C
handleRoutine _isr6D, 0x6D
handleRoutine _isr6E, 0x6E
handleRoutine _isr6F, 0x6F
handleRoutine _isr70, 0x70
handleRoutine _isr71, 0x71
handleRoutine _isr72, 0x72
handleRoutine _isr73, 0x73
handleRoutine _isr74, 0x74
handleRoutine _isr75, 0x75
handleRoutine _isr76, 0x76
handleRoutine _isr77, 0x77
handleRoutine _isr78, 0x78
handleRoutine _isr79, 0x79
handleRoutine _isr7A, 0x7A
handleRoutine _isr7B, 0x7B
handleRoutine _isr7C, 0x7C
handleRoutine _isr7D, 0x7D
handleRoutine _isr7E, 0x7E
handleRoutine _isr7F, 0x7F
handleRoutine _isr80, 0x80
handleRoutine _isr81, 0x81
handleRoutine _isr82, 0x82
handleRoutine _isr83, 0x83
handleRoutine _isr84, 0x84
handleRoutine _isr85, 0x85
handleRoutine _isr86, 0x86
handleRoutine _isr87, 0x87
handleRoutine _isr88, 0x88
handleRoutine _isr89, 0x89
handleRoutine _isr8A, 0x8A
handleRoutine _isr8B, 0x8B
handleRoutine _isr8C, 0x8C
handleRoutine _isr8D, 0x8D
handleRoutine _isr8E, 0x8E
handleRoutine _isr8F, 0x8F
handleRoutine _isr90, 0x90
handleRoutine _isr91, 0x91
handleRoutine _isr92, 0x92
handleRoutine _isr93, 0x93
handleRoutine _isr94, 0x94
handleRoutine _isr95, 0x95
handleRoutine _isr96, 0x96
handleRoutine _isr97, 0x97
handleRoutine _isr98, 0x98
handleRoutine _isr99, 0x99
handleRoutine _isr9A, 0x9A
handleRoutine _isr9B, 0x9B
handleRoutine _isr9C, 0x9C
handleRoutine _isr9D, 0x9D
handleRoutine _isr9E, 0x9E
handleRoutine _isr9F, 0x9F
handleRoutine _isrA0, 0xA0
handleRoutine _isrA1, 0xA1
handleRoutine _isrA2, 0xA2
handleRoutine _isrA3, 0xA3
handleRoutine _isrA4, 0xA4
handleRoutine _isrA5, 0xA5
handleRoutine _isrA6, 0xA6
handleRoutine _isrA7, 0xA7
handleRoutine _isrA8, 0xA8
handleRoutine _isrA9, 0xA9
handleRoutine _isrAA, 0xAA
handleRoutine _isrAB, 0xAB
handleRoutine _isrAC, 0xAC
handleRoutine _isrAD, 0xAD
handleRoutine _isrAE, 0xAE
handleRoutine _isrAF, 0xAF
handleRoutine _isrB0, 0xB0
handleRoutine _isrB1, 0xB1
handleRoutine _isrB2, 0xB2
handleRoutine _isrB3, 0xB3
handleRoutine _isrB4, 0xB4
handleRoutine _isrB5, 0xB5
handleRoutine _isrB6, 0xB6
handleRoutine _isrB7, 0xB7
handleRoutine _isrB8, 0xB8
handleRoutine _isrB9, 0xB9
handleRoutine _isrBA, 0xBA
handleRoutine _isrBB, 0xBB
handleRoutine _isrBC, 0xBC
handleRoutine _isrBD, 0xBD
handleRoutine _isrBE, 0xBE
handleRoutine _isrBF, 0xBF
handleRoutine _isrC0, 0xC0
handleRoutine _isrC1, 0xC1
handleRoutine _isrC2, 0xC2
handleRoutine _isrC3, 0xC3
handleRoutine _isrC4, 0xC4
handleRoutine _isrC5, 0xC5
handleRoutine _isrC6, 0xC6
handleRoutine _isrC7, 0xC7
handleRoutine _isrC8, 0xC8
handleRoutine _isrC9, 0xC9
handleRoutine _isrCA, 0xCA
handleRoutine _isrCB, 0xCB
handleRoutine _isrCC, 0xCC
handleRoutine _isrCD, 0xCD
handleRoutine _isrCE, 0xCE
handleRoutine _isrCF, 0xCF
handleRoutine _isrD0, 0xD0
handleRoutine _isrD1, 0xD1
handleRoutine _isrD2, 0xD2
handleRoutine _isrD3, 0xD3
handleRoutine _isrD4, 0xD4
handleRoutine _isrD5, 0xD5
handleRoutine _isrD6, 0xD6
handleRoutine _isrD7, 0xD7
handleRoutine _isrD8, 0xD8
handleRoutine _isrD9, 0xD9
handleRoutine _isrDA, 0xDA
handleRoutine _isrDB, 0xDB
handleRoutine _isrDC, 0xDC
handleRoutine _isrDD, 0xDD
handleRoutine _isrDE, 0xDE
handleRoutine _isrDF, 0xDF
handleRoutine _isrE0, 0xE0
handleRoutine _isrE1, 0xE1
handleRoutine _isrE2, 0xE2
handleRoutine _isrE3, 0xE3
handleRoutine _isrE4, 0xE4
handleRoutine _isrE5, 0xE5
handleRoutine _isrE6, 0xE6
handleRoutine _isrE7, 0xE7
handleRoutine _isrE8, 0xE8
handleRoutine _isrE9, 0xE9
handleRoutine _isrEA, 0xEA
handleRoutine _isrEB, 0xEB
handleRoutine _isrEC, 0xEC
handleRoutine _isrED, 0xED
handleRoutine _isrEE, 0xEE
handleRoutine _isrEF, 0xEF
handleRoutine _isrF0, 0xF0
handleRoutine _isrF1, 0xF1
handleRoutine _isrF2, 0xF2
handleRoutine _isrF3, 0xF3
handleRoutine _isrF4, 0xF4
handleRoutine _isrF5, 0xF5
handleRoutine _isrF6, 0xF6
handleRoutine _isrF7, 0xF7
handleRoutine _isrF8, 0xF8
handleRoutine _isrF9, 0xF9
handleRoutine _isrFA, 0xFA
handleRoutine _isrFB, 0xFB
handleRoutine _isrFC, 0xFC
handleRoutine _isrFD, 0xFD
handleRoutine _isrFE, 0xFE
handleRoutine _isrFF, 0xFF
