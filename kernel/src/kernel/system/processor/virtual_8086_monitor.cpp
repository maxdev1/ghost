/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "kernel/system/processor/virtual_8086_monitor.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/system/io_port.hpp"
#include "kernel/system/interrupts/ivt.hpp"


g_virtual_monitor_handling_result vm86MonitorHandleGpf(g_task* task)
{
	g_processor_state_vm86* ctx = (g_processor_state_vm86*) task->state;
	uint8_t* ip = (uint8_t*) G_SEGOFF_TO_LINEAR(ctx->defaultFrame.cs, ctx->defaultFrame.eip);
	uint16_t* sp = (uint16_t*) G_SEGOFF_TO_LINEAR(ctx->defaultFrame.ss, ctx->defaultFrame.esp);
	uint32_t* esp = (uint32_t*) sp;

	bool operands32 = false;
	bool address32 = false;

	while (true) {

		switch (ip[0]) {
		/**
		 * Enables 32bit operands for the next instructions
		 */
		case 0x66: {
			operands32 = true;
			++ip;
			++ctx->defaultFrame.eip;
			break;
		}

			/**
			 * Enables 32bit addresses for the next instruction
			 */
		case 0x67: {
			address32 = true;
			++ip;
			++ctx->defaultFrame.eip;
			break;
		}

			/**
			 * Instruction 0x9C:
			 *   PUSHF
			 *
			 * Pushes the CPU's eflags
			 */
		case 0x9C: {

			if (operands32) {
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) - 4) & 0xffff;
				esp--;
				esp[0] = ctx->defaultFrame.eflags & G_VALID_FLAGS;

				if (task->vm86Data->cpuIf) {
					esp[0] |= G_EFLAG_IF;
				} else {
					esp[0] &= ~G_EFLAG_IF;
				}
			} else {
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) - 2) & 0xffff;
				sp--;
				sp[0] = (uint16_t) ctx->defaultFrame.eflags;

				if (task->vm86Data->cpuIf) {
					sp[0] |= G_EFLAG_IF;
				} else {
					sp[0] &= ~G_EFLAG_IF;
				}
			}

			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0x9D:
			 *   POPF
			 *
			 * Pops the CPU's eflags
			 */
		case 0x9D: {

			if (operands32) {
				ctx->defaultFrame.eflags = G_EFLAG_IF | G_EFLAG_VM | (esp[0] & G_VALID_FLAGS);
				task->vm86Data->cpuIf = (esp[0] & G_EFLAG_IF) != 0;
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) + 4) & 0xffff;
			} else {
				ctx->defaultFrame.eflags = G_EFLAG_IF | G_EFLAG_VM | sp[0];
				task->vm86Data->cpuIf = (sp[0] & G_EFLAG_IF) != 0;
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) + 2) & 0xffff;
			}

			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xCD:
			 *   INT x
			 *
			 * Calls an interrupt
			 */
		case 0xCD: {
			sp -= 3;
			ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) - 6) & 0xffff;

			sp[0] = (uint16_t) (ctx->defaultFrame.eip + 2);
			sp[1] = ctx->defaultFrame.cs;
			sp[2] = (uint16_t) ctx->defaultFrame.eflags;

			if (task->vm86Data->cpuIf) {
				sp[2] |= G_EFLAG_IF;
			} else {
				sp[2] &= ~G_EFLAG_IF;
			}

			ctx->defaultFrame.cs = G_FP_SEG(ivt->entry[ip[1]]);
			ctx->defaultFrame.eip = G_FP_OFF(ivt->entry[ip[1]]);

			++task->vm86Data->interruptRecursionLevel;

			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xCF:
			 *   IRET
			 *
			 * Returns from an interrupt
			 */
		case 0xCF: {
			ctx->defaultFrame.eip = sp[0];
			ctx->defaultFrame.cs = sp[1];
			ctx->defaultFrame.eflags = G_EFLAG_IF | G_EFLAG_VM | sp[2];
			task->vm86Data->cpuIf = ((sp[2] & G_EFLAG_IF) != 0);

			ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) + 6) & 0xffff;

			if (task->vm86Data->interruptRecursionLevel == 0) {
				task->vm86Data->out->ax = ctx->defaultFrame.eax;
				task->vm86Data->out->bx = ctx->defaultFrame.ebx;
				task->vm86Data->out->cx = ctx->defaultFrame.ecx;
				task->vm86Data->out->dx = ctx->defaultFrame.edx;

				task->vm86Data->out->di = ctx->defaultFrame.edi;
				task->vm86Data->out->si = ctx->defaultFrame.esi;
				task->vm86Data->out->ds = ctx->ds;
				task->vm86Data->out->es = ctx->es;

				return VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED;
			}

			--task->vm86Data->interruptRecursionLevel;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xFA:
			 *   CLI
			 *
			 * Disables interrupts
			 */
		case 0xFA: {
			task->vm86Data->cpuIf = false;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xFB:
			 *   STI
			 *
			 * Enables interrupts
			 */
		case 0xFB: {
			task->vm86Data->cpuIf = true;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/* Instruction 0xEE:
			 *   OUT dx, al
			 *
			 * Output byte in AL to I/O port address in DX.
			 */
		case 0xEE: {
			ioPortWriteByte((uint16_t) ctx->defaultFrame.edx, (uint8_t) ctx->defaultFrame.eax);
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/* Instruction 0xEF:
			 *   OUT dx, ax
			 *
			 * Output word in AX to I/O port address in DX.
			 */
		case 0xEF: {
			ioPortWriteShort((uint16_t) ctx->defaultFrame.edx, (uint16_t) ctx->defaultFrame.eax);
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xEC:
			 *   IN al, dx
			 *
			 * Input byte from I/O port in DX into AL.
			 */
		case 0xEC: {
			uint8_t res = ioPortReadByte((uint16_t) ctx->defaultFrame.edx);
			ctx->defaultFrame.eax &= ~(0xFF);
			ctx->defaultFrame.eax |= res;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xED:
			 *   IN al, dx
			 *
			 * Input word from I/O port in DX into AX.
			 */
		case 0xED: {
			uint16_t res = ioPortReadShort((uint16_t) ctx->defaultFrame.edx);
			ctx->defaultFrame.eax &= ~(0xFFFF);
			ctx->defaultFrame.eax |= res;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Unhandled operation
			 */
		default: {
			logInfo("%! unhandled opcode %h at linear location %h", "vm86", (uint32_t ) ip[0], ip);
			return VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE;
		}
		}
	}

	// Not reached
	return VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE;
}
