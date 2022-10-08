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

;
; C handler functions
;
extern _interruptHandler

;
; Handler routine
;
interruptRoutine:
	;
	; The processor has now pushed registers to the stack. If it was an actual
	; context switch, then SS and ESP are also pushed. For a same-ring switch
	; it will not push them. Roughly equivalent to these instructions:
	;
	; [Ring 3 -> Ring 0]
	;	push ss
	;	push esp
	;	push eflags
	;	push cs
	;	push eip
	;
	; [Ring 0 -> Ring 0]
	;	push eflags
	;	push cs
	;	push eip
	;
	; This is the reason we give the stack pointer to our interrupt handler.
	; The interrupt handler will then return the stack that we can pop the
	; registers from.
	;

	; Store general purpose
	push edi
	push esi
	push ebp
	push ebx
	push edx
	push ecx
	push eax

	; Store segments
	push ds
	push es
	push fs
	push gs

	; Switch to kernel segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax
	; Segment points to kernel thread-local data
	mov ax, 0x38
	mov gs, ax

	; Stack pointer argument
	push esp
	; Call handler
	call _interruptHandler
	; Set stack from return value
	mov esp, eax

	; Restore segments
	pop gs
	pop fs
	pop es
	pop ds

	; Restore general purpose
	pop eax
	pop ecx
	pop edx
	pop ebx
	pop ebp
	pop esi
	pop edi

	; Skip intr and error in Registers struct
	add esp, 8

	;
	; Now we return and on IRET the processor again pops specific registers.
	; If we switch to a kernel-level task, ESP and SS will not be popped.
	;
	; [Ring 0 -> Ring 0]
	;		pop eip
	;		pop cs
	;		pop eflags
	;
	; [Ring 0 -> Ring 3]
	;		pop eip
	;		pop cs
	;		pop eflags
	;		pop esp
	;		pop ss
	;
	iret


; Handling routine macros
%macro handleRoutineErr 2
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
handleRoutine _irout0,  0  		; Divide by zero exception
handleRoutine _irout1,  1  		; Debug exception
handleRoutine _irout2,  2  		; Non-maskable interrupt exception
handleRoutine _irout3,  3  		; Breakpoint exception
handleRoutine _irout4,  4  		; Detected overflow exception
handleRoutine _irout5,  5  		; Out of bounds exception
handleRoutine _irout6,  6  		; Invalid operation code exception
handleRoutine _irout7,  7  		; No coprocessor exception
handleRoutineErr _irout8,  8    ; Double fault exception [has error code]
handleRoutine _irout9,  9  		; Coprocessor segment overrun exception
handleRoutineErr _irout10, 10   ; Bad TSS exception [has error code]
handleRoutineErr _irout11, 11   ; Segment not present exception [has error code]
handleRoutineErr _irout12, 12   ; Stack fault exception [has error code]
handleRoutineErr _irout13, 13   ; General protection fault exception [has error code]
handleRoutineErr _irout14, 14   ; Page fault [has error code]
handleRoutine _irout15, 15 		; Unknown interrupt exception
handleRoutine _irout16, 16 		; Coprocessor fault exception
handleRoutine _irout17, 17 		; Alignment check exception
handleRoutine _irout18, 18 		; Machine check exception

; Reserved exceptions
handleRoutine _irout19, 19
handleRoutine _irout20, 20
handleRoutine _irout21, 21
handleRoutine _irout22, 22
handleRoutine _irout23, 23
handleRoutine _irout24, 24
handleRoutine _irout25, 25
handleRoutine _irout26, 26
handleRoutine _irout27, 27
handleRoutine _irout28, 28
handleRoutine _irout29, 29
handleRoutine _irout30, 30
handleRoutine _irout31, 31

; Requests
handleRoutine _ireq0,  32
handleRoutine _ireq1,  33
handleRoutine _ireq2,  34
handleRoutine _ireq3,  35
handleRoutine _ireq4,  36
handleRoutine _ireq5,  37
handleRoutine _ireq6,  38
handleRoutine _ireq7,  39
handleRoutine _ireq8,  40
handleRoutine _ireq9,  41
handleRoutine _ireq10, 42
handleRoutine _ireq11, 43
handleRoutine _ireq12, 44
handleRoutine _ireq13, 45
handleRoutine _ireq14, 46
handleRoutine _ireq15, 47
handleRoutine _ireq16, 48
handleRoutine _ireq17, 49
handleRoutine _ireq18, 50
handleRoutine _ireq19, 51
handleRoutine _ireq20, 52
handleRoutine _ireq21, 53
handleRoutine _ireq22, 54
handleRoutine _ireq23, 55
handleRoutine _ireq24, 56
handleRoutine _ireq25, 57
handleRoutine _ireq26, 58
handleRoutine _ireq27, 59
handleRoutine _ireq28, 60
handleRoutine _ireq29, 61
handleRoutine _ireq30, 62
handleRoutine _ireq31, 63
handleRoutine _ireq32, 64
handleRoutine _ireq33, 65
handleRoutine _ireq34, 66
handleRoutine _ireq35, 67
handleRoutine _ireq36, 68
handleRoutine _ireq37, 69
handleRoutine _ireq38, 70
handleRoutine _ireq39, 71
handleRoutine _ireq40, 72
handleRoutine _ireq41, 73
handleRoutine _ireq42, 74
handleRoutine _ireq43, 75
handleRoutine _ireq44, 76
handleRoutine _ireq45, 77
handleRoutine _ireq46, 78
handleRoutine _ireq47, 79
handleRoutine _ireq48, 80
handleRoutine _ireq49, 81
handleRoutine _ireq50, 82
handleRoutine _ireq51, 83
handleRoutine _ireq52, 84
handleRoutine _ireq53, 85
handleRoutine _ireq54, 86
handleRoutine _ireq55, 87
handleRoutine _ireq56, 88
handleRoutine _ireq57, 89
handleRoutine _ireq58, 90
handleRoutine _ireq59, 91
handleRoutine _ireq60, 92
handleRoutine _ireq61, 93
handleRoutine _ireq62, 94
handleRoutine _ireq63, 95
handleRoutine _ireq64, 96
handleRoutine _ireq65, 97
handleRoutine _ireq66, 98
handleRoutine _ireq67, 99
handleRoutine _ireq68, 100
handleRoutine _ireq69, 101
handleRoutine _ireq70, 102
handleRoutine _ireq71, 103
handleRoutine _ireq72, 104
handleRoutine _ireq73, 105
handleRoutine _ireq74, 106
handleRoutine _ireq75, 107
handleRoutine _ireq76, 108
handleRoutine _ireq77, 109
handleRoutine _ireq78, 110
handleRoutine _ireq79, 111
handleRoutine _ireq80, 112
handleRoutine _ireq81, 113
handleRoutine _ireq82, 114
handleRoutine _ireq83, 115
handleRoutine _ireq84, 116
handleRoutine _ireq85, 117
handleRoutine _ireq86, 118
handleRoutine _ireq87, 119
handleRoutine _ireq88, 120
handleRoutine _ireq89, 121
handleRoutine _ireq90, 122
handleRoutine _ireq91, 123
handleRoutine _ireq92, 124
handleRoutine _ireq93, 125
handleRoutine _ireq94, 126
handleRoutine _ireq95, 127
handleRoutine _ireqSyscall, 128
handleRoutine _ireq97, 129
handleRoutine _ireq98, 130
handleRoutine _ireq99, 131
handleRoutine _ireq100, 132
handleRoutine _ireq101, 133
handleRoutine _ireq102, 134
handleRoutine _ireq103, 135
handleRoutine _ireq104, 136
handleRoutine _ireq105, 137
handleRoutine _ireq106, 138
handleRoutine _ireq107, 139
handleRoutine _ireq108, 140
handleRoutine _ireq109, 141
handleRoutine _ireq110, 142
handleRoutine _ireq111, 143
handleRoutine _ireq112, 144
handleRoutine _ireq113, 145
handleRoutine _ireq114, 146
handleRoutine _ireq115, 147
handleRoutine _ireq116, 148
handleRoutine _ireq117, 149
handleRoutine _ireq118, 150
handleRoutine _ireq119, 151
handleRoutine _ireq120, 152
handleRoutine _ireq121, 153
handleRoutine _ireq122, 154
handleRoutine _ireq123, 155
handleRoutine _ireq124, 156
handleRoutine _ireq125, 157
handleRoutine _ireq126, 158
handleRoutine _ireq127, 159
handleRoutine _ireq128, 160
handleRoutine _ireq129, 161
handleRoutine _ireq130, 162
handleRoutine _ireq131, 163
handleRoutine _ireq132, 164
handleRoutine _ireq133, 165
handleRoutine _ireq134, 166
handleRoutine _ireq135, 167
handleRoutine _ireq136, 168
handleRoutine _ireq137, 169
handleRoutine _ireq138, 170
handleRoutine _ireq139, 171
handleRoutine _ireq140, 172
handleRoutine _ireq141, 173
handleRoutine _ireq142, 174
handleRoutine _ireq143, 175
handleRoutine _ireq144, 176
handleRoutine _ireq145, 177
handleRoutine _ireq146, 178
handleRoutine _ireq147, 179
handleRoutine _ireq148, 180
handleRoutine _ireq149, 181
handleRoutine _ireq150, 182
handleRoutine _ireq151, 183
handleRoutine _ireq152, 184
handleRoutine _ireq153, 185
handleRoutine _ireq154, 186
handleRoutine _ireq155, 187
handleRoutine _ireq156, 188
handleRoutine _ireq157, 189
handleRoutine _ireq158, 190
handleRoutine _ireq159, 191
handleRoutine _ireq160, 192
handleRoutine _ireq161, 193
handleRoutine _ireq162, 194
handleRoutine _ireq163, 195
handleRoutine _ireq164, 196
handleRoutine _ireq165, 197
handleRoutine _ireq166, 198
handleRoutine _ireq167, 199
handleRoutine _ireq168, 200
handleRoutine _ireq169, 201
handleRoutine _ireq170, 202
handleRoutine _ireq171, 203
handleRoutine _ireq172, 204
handleRoutine _ireq173, 205
handleRoutine _ireq174, 206
handleRoutine _ireq175, 207
handleRoutine _ireq176, 208
handleRoutine _ireq177, 209
handleRoutine _ireq178, 210
handleRoutine _ireq179, 211
handleRoutine _ireq180, 212
handleRoutine _ireq181, 213
handleRoutine _ireq182, 214
handleRoutine _ireq183, 215
handleRoutine _ireq184, 216
handleRoutine _ireq185, 217
handleRoutine _ireq186, 218
handleRoutine _ireq187, 219
handleRoutine _ireq188, 220
handleRoutine _ireq189, 221
handleRoutine _ireq190, 222
handleRoutine _ireq191, 223
handleRoutine _ireq192, 224
handleRoutine _ireq193, 225
handleRoutine _ireq194, 226
handleRoutine _ireq195, 227
handleRoutine _ireq196, 228
handleRoutine _ireq197, 229
handleRoutine _ireq198, 230
handleRoutine _ireq199, 231
handleRoutine _ireq200, 232
handleRoutine _ireq201, 233
handleRoutine _ireq202, 234
handleRoutine _ireq203, 235
handleRoutine _ireq204, 236
handleRoutine _ireq205, 237
handleRoutine _ireq206, 238
handleRoutine _ireq207, 239
handleRoutine _ireq208, 240
handleRoutine _ireq209, 241
handleRoutine _ireq210, 242
handleRoutine _ireq211, 243
handleRoutine _ireq212, 244
handleRoutine _ireq213, 245
handleRoutine _ireq214, 246
handleRoutine _ireq215, 247
handleRoutine _ireq216, 248
handleRoutine _ireq217, 249
handleRoutine _ireq218, 250
handleRoutine _ireq219, 251
handleRoutine _ireq220, 252
handleRoutine _ireq221, 253
handleRoutine _ireq222, 254
handleRoutine _ireq223, 255
