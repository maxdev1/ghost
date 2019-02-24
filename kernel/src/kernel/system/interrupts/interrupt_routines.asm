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
	mov gs, ax

	; Push stack pointer
	push esp
	; Call handler
	call _interruptHandler
	; Set stack pointer to returned value
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

	; Restore rest
	iret


; Handle routine macro for interrupts with error code
%macro handleRoutine 2
global %1
%1:
	cli
	push %2
	jmp interruptRoutine
%endmacro

; Handle routine macro for interrupts without error code
%macro handleRoutine 3
global %1
%1:
	cli
	push %2
	push %3
	jmp interruptRoutine
%endmacro


; Exceptions
handleRoutine _irout0,  0, 0  ; Divide by zero exception
handleRoutine _irout1,  0, 1  ; Debug exception
handleRoutine _irout2,  0, 2  ; Non-maskable interrupt exception
handleRoutine _irout3,  0, 3  ; Breakpoint exception
handleRoutine _irout4,  0, 4  ; Detected overflow exception
handleRoutine _irout5,  0, 5  ; Out of bounds exception
handleRoutine _irout6,  0, 6  ; Invalid operation code exception
handleRoutine _irout7,  0, 7  ; No coprocessor exception
handleRoutine _irout8,  8     ; Double fault exception [has error code]
handleRoutine _irout9,  0, 9  ; Coprocessor segment overrun exception
handleRoutine _irout10, 10    ; Bad TSS exception [has error code]
handleRoutine _irout11, 11    ; Segment not present exception [has error code]
handleRoutine _irout12, 12    ; Stack fault exception [has error code]
handleRoutine _irout13, 13    ; General protection fault exception [has error code]
handleRoutine _irout14, 14    ; Page fault [has error code]
handleRoutine _irout15, 0, 15 ; Unknown interrupt exception
handleRoutine _irout16, 0, 16 ; Coprocessor fault exception
handleRoutine _irout17, 0, 17 ; Alignment check exception
handleRoutine _irout18, 0, 18 ; Machine check exception

; Reserved exceptions
handleRoutine _irout19, 0, 19
handleRoutine _irout20, 0, 20
handleRoutine _irout21, 0, 21
handleRoutine _irout22, 0, 22
handleRoutine _irout23, 0, 23
handleRoutine _irout24, 0, 24
handleRoutine _irout25, 0, 25
handleRoutine _irout26, 0, 26
handleRoutine _irout27, 0, 27
handleRoutine _irout28, 0, 28
handleRoutine _irout29, 0, 29
handleRoutine _irout30, 0, 30
handleRoutine _irout31, 0, 31

; Requests
handleRoutine _ireq0,  0, 32
handleRoutine _ireq1,  0, 33
handleRoutine _ireq2,  0, 34
handleRoutine _ireq3,  0, 35
handleRoutine _ireq4,  0, 36
handleRoutine _ireq5,  0, 37
handleRoutine _ireq6,  0, 38
handleRoutine _ireq7,  0, 39
handleRoutine _ireq8,  0, 40
handleRoutine _ireq9,  0, 41
handleRoutine _ireq10, 0, 42
handleRoutine _ireq11, 0, 43
handleRoutine _ireq12, 0, 44
handleRoutine _ireq13, 0, 45
handleRoutine _ireq14, 0, 46
handleRoutine _ireq15, 0, 47
handleRoutine _ireq16, 0, 48
handleRoutine _ireq17, 0, 49
handleRoutine _ireq18, 0, 50
handleRoutine _ireq19, 0, 51
handleRoutine _ireq20, 0, 52
handleRoutine _ireq21, 0, 53
handleRoutine _ireq22, 0, 54
handleRoutine _ireq23, 0, 55
handleRoutine _ireq24, 0, 56
handleRoutine _ireq25, 0, 57
handleRoutine _ireq26, 0, 58
handleRoutine _ireq27, 0, 59
handleRoutine _ireq28, 0, 60
handleRoutine _ireq29, 0, 61
handleRoutine _ireq30, 0, 62
handleRoutine _ireq31, 0, 63
handleRoutine _ireq32, 0, 64
handleRoutine _ireq33, 0, 65
handleRoutine _ireq34, 0, 66
handleRoutine _ireq35, 0, 67
handleRoutine _ireq36, 0, 68
handleRoutine _ireq37, 0, 69
handleRoutine _ireq38, 0, 70
handleRoutine _ireq39, 0, 71
handleRoutine _ireq40, 0, 72
handleRoutine _ireq41, 0, 73
handleRoutine _ireq42, 0, 74
handleRoutine _ireq43, 0, 75
handleRoutine _ireq44, 0, 76
handleRoutine _ireq45, 0, 77
handleRoutine _ireq46, 0, 78
handleRoutine _ireq47, 0, 79
handleRoutine _ireq48, 0, 80
handleRoutine _ireq49, 0, 81
handleRoutine _ireq50, 0, 82
handleRoutine _ireq51, 0, 83
handleRoutine _ireq52, 0, 84
handleRoutine _ireq53, 0, 85
handleRoutine _ireq54, 0, 86
handleRoutine _ireq55, 0, 87
handleRoutine _ireq56, 0, 88
handleRoutine _ireq57, 0, 89
handleRoutine _ireq58, 0, 90
handleRoutine _ireq59, 0, 91
handleRoutine _ireq60, 0, 92
handleRoutine _ireq61, 0, 93
handleRoutine _ireq62, 0, 94
handleRoutine _ireq63, 0, 95
handleRoutine _ireq64, 0, 96
handleRoutine _ireq65, 0, 97
handleRoutine _ireq66, 0, 98
handleRoutine _ireq67, 0, 99
handleRoutine _ireq68, 0, 100
handleRoutine _ireq69, 0, 101
handleRoutine _ireq70, 0, 102
handleRoutine _ireq71, 0, 103
handleRoutine _ireq72, 0, 104
handleRoutine _ireq73, 0, 105
handleRoutine _ireq74, 0, 106
handleRoutine _ireq75, 0, 107
handleRoutine _ireq76, 0, 108
handleRoutine _ireq77, 0, 109
handleRoutine _ireq78, 0, 110
handleRoutine _ireq79, 0, 111
handleRoutine _ireq80, 0, 112
handleRoutine _ireq81, 0, 113
handleRoutine _ireq82, 0, 114
handleRoutine _ireq83, 0, 115
handleRoutine _ireq84, 0, 116
handleRoutine _ireq85, 0, 117
handleRoutine _ireq86, 0, 118
handleRoutine _ireq87, 0, 119
handleRoutine _ireq88, 0, 120
handleRoutine _ireq89, 0, 121
handleRoutine _ireq90, 0, 122
handleRoutine _ireq91, 0, 123
handleRoutine _ireq92, 0, 124
handleRoutine _ireq93, 0, 125
handleRoutine _ireq94, 0, 126
handleRoutine _ireq95, 0, 127
handleRoutine _ireqSyscall, 0, 128
handleRoutine _ireq97, 0, 129
handleRoutine _ireq98, 0, 130
handleRoutine _ireq99, 0, 131
handleRoutine _ireq100, 0, 132
handleRoutine _ireq101, 0, 133
handleRoutine _ireq102, 0, 134
handleRoutine _ireq103, 0, 135
handleRoutine _ireq104, 0, 136
handleRoutine _ireq105, 0, 137
handleRoutine _ireq106, 0, 138
handleRoutine _ireq107, 0, 139
handleRoutine _ireq108, 0, 140
handleRoutine _ireq109, 0, 141
handleRoutine _ireq110, 0, 142
handleRoutine _ireq111, 0, 143
handleRoutine _ireq112, 0, 144
handleRoutine _ireq113, 0, 145
handleRoutine _ireq114, 0, 146
handleRoutine _ireq115, 0, 147
handleRoutine _ireq116, 0, 148
handleRoutine _ireq117, 0, 149
handleRoutine _ireq118, 0, 150
handleRoutine _ireq119, 0, 151
handleRoutine _ireq120, 0, 152
handleRoutine _ireq121, 0, 153
handleRoutine _ireq122, 0, 154
handleRoutine _ireq123, 0, 155
handleRoutine _ireq124, 0, 156
handleRoutine _ireq125, 0, 157
handleRoutine _ireq126, 0, 158
handleRoutine _ireq127, 0, 159
handleRoutine _ireq128, 0, 160
handleRoutine _ireq129, 0, 161
handleRoutine _ireq130, 0, 162
handleRoutine _ireq131, 0, 163
handleRoutine _ireq132, 0, 164
handleRoutine _ireq133, 0, 165
handleRoutine _ireq134, 0, 166
handleRoutine _ireq135, 0, 167
handleRoutine _ireq136, 0, 168
handleRoutine _ireq137, 0, 169
handleRoutine _ireq138, 0, 170
handleRoutine _ireq139, 0, 171
handleRoutine _ireq140, 0, 172
handleRoutine _ireq141, 0, 173
handleRoutine _ireq142, 0, 174
handleRoutine _ireq143, 0, 175
handleRoutine _ireq144, 0, 176
handleRoutine _ireq145, 0, 177
handleRoutine _ireq146, 0, 178
handleRoutine _ireq147, 0, 179
handleRoutine _ireq148, 0, 180
handleRoutine _ireq149, 0, 181
handleRoutine _ireq150, 0, 182
handleRoutine _ireq151, 0, 183
handleRoutine _ireq152, 0, 184
handleRoutine _ireq153, 0, 185
handleRoutine _ireq154, 0, 186
handleRoutine _ireq155, 0, 187
handleRoutine _ireq156, 0, 188
handleRoutine _ireq157, 0, 189
handleRoutine _ireq158, 0, 190
handleRoutine _ireq159, 0, 191
handleRoutine _ireq160, 0, 192
handleRoutine _ireq161, 0, 193
handleRoutine _ireq162, 0, 194
handleRoutine _ireq163, 0, 195
handleRoutine _ireq164, 0, 196
handleRoutine _ireq165, 0, 197
handleRoutine _ireq166, 0, 198
handleRoutine _ireq167, 0, 199
handleRoutine _ireq168, 0, 200
handleRoutine _ireq169, 0, 201
handleRoutine _ireq170, 0, 202
handleRoutine _ireq171, 0, 203
handleRoutine _ireq172, 0, 204
handleRoutine _ireq173, 0, 205
handleRoutine _ireq174, 0, 206
handleRoutine _ireq175, 0, 207
handleRoutine _ireq176, 0, 208
handleRoutine _ireq177, 0, 209
handleRoutine _ireq178, 0, 210
handleRoutine _ireq179, 0, 211
handleRoutine _ireq180, 0, 212
handleRoutine _ireq181, 0, 213
handleRoutine _ireq182, 0, 214
handleRoutine _ireq183, 0, 215
handleRoutine _ireq184, 0, 216
handleRoutine _ireq185, 0, 217
handleRoutine _ireq186, 0, 218
handleRoutine _ireq187, 0, 219
handleRoutine _ireq188, 0, 220
handleRoutine _ireq189, 0, 221
handleRoutine _ireq190, 0, 222
handleRoutine _ireq191, 0, 223
handleRoutine _ireq192, 0, 224
handleRoutine _ireq193, 0, 225
handleRoutine _ireq194, 0, 226
handleRoutine _ireq195, 0, 227
handleRoutine _ireq196, 0, 228
handleRoutine _ireq197, 0, 229
handleRoutine _ireq198, 0, 230
handleRoutine _ireq199, 0, 231
handleRoutine _ireq200, 0, 232
handleRoutine _ireq201, 0, 233
handleRoutine _ireq202, 0, 234
handleRoutine _ireq203, 0, 235
handleRoutine _ireq204, 0, 236
handleRoutine _ireq205, 0, 237
handleRoutine _ireq206, 0, 238
handleRoutine _ireq207, 0, 239
handleRoutine _ireq208, 0, 240
handleRoutine _ireq209, 0, 241
handleRoutine _ireq210, 0, 242
handleRoutine _ireq211, 0, 243
handleRoutine _ireq212, 0, 244
handleRoutine _ireq213, 0, 245
handleRoutine _ireq214, 0, 246
handleRoutine _ireq215, 0, 247
handleRoutine _ireq216, 0, 248
handleRoutine _ireq217, 0, 249
handleRoutine _ireq218, 0, 250
handleRoutine _ireq219, 0, 251
handleRoutine _ireq220, 0, 252
handleRoutine _ireq221, 0, 253
handleRoutine _ireq222, 0, 254
handleRoutine _ireq223, 0, 255
