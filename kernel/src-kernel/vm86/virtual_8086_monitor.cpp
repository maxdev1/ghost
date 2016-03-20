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

#include <vm86/virtual_8086_monitor.hpp>
#include <memory/memory.hpp>
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread.hpp>
#include <system/io_ports.hpp>
#include <system/interrupts/descriptors/ivt.hpp>

/**
 *
 */
g_virtual_monitor_handling_result g_virtual_8086_monitor::handleGpf(g_thread* current) {
	g_processor_state_vm86* ctx = (g_processor_state_vm86*) current->cpuState;
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

				if (current->getVm86Information()->cpuIf) {
					esp[0] |= G_EFLAG_IF;
				} else {
					esp[0] &= ~G_EFLAG_IF;
				}
			} else {
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) - 2) & 0xffff;
				sp--;
				sp[0] = (uint16_t) ctx->defaultFrame.eflags;

				if (current->getVm86Information()->cpuIf) {
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
				current->getVm86Information()->cpuIf = (esp[0] & G_EFLAG_IF) != 0;
				ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) + 4) & 0xffff;
			} else {
				ctx->defaultFrame.eflags = G_EFLAG_IF | G_EFLAG_VM | sp[0];
				current->getVm86Information()->cpuIf = (sp[0] & G_EFLAG_IF) != 0;
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

			if (current->getVm86Information()->cpuIf) {
				sp[2] |= G_EFLAG_IF;
			} else {
				sp[2] &= ~G_EFLAG_IF;
			}

			ctx->defaultFrame.cs = G_FP_SEG(ivt->entry[ip[1]]);
			ctx->defaultFrame.eip = G_FP_OFF(ivt->entry[ip[1]]);

			++current->getVm86Information()->interruptRecursionLevel;

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
			current->getVm86Information()->cpuIf = ((sp[2] & G_EFLAG_IF) != 0);

			ctx->defaultFrame.esp = ((ctx->defaultFrame.esp & 0xffff) + 6) & 0xffff;

			if (current->getVm86Information()->interruptRecursionLevel == 0) {
				current->getVm86Information()->out->ax = ctx->defaultFrame.eax;
				current->getVm86Information()->out->bx = ctx->defaultFrame.ebx;
				current->getVm86Information()->out->cx = ctx->defaultFrame.ecx;
				current->getVm86Information()->out->dx = ctx->defaultFrame.edx;

				current->getVm86Information()->out->di = ctx->defaultFrame.edi;
				current->getVm86Information()->out->si = ctx->defaultFrame.esi;
				current->getVm86Information()->out->ds = ctx->ds;
				current->getVm86Information()->out->es = ctx->es;

				return VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED;
			}

			--current->getVm86Information()->interruptRecursionLevel;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Instruction 0xFA:
			 *   CLI
			 *
			 * Disables interrupts
			 */
		case 0xFA: {
			current->getVm86Information()->cpuIf = false;
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
			current->getVm86Information()->cpuIf = true;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/* Instruction 0xEE:
			 *   OUT dx, al
			 *
			 * Output byte in AL to I/O port address in DX.
			 */
		case 0xEE: {
			g_io_ports::writeByte((uint16_t) ctx->defaultFrame.edx, (uint8_t) ctx->defaultFrame.eax);
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/* Instruction 0xEF:
			 *   OUT dx, ax
			 *
			 * Output word in AX to I/O port address in DX.
			 */
		case 0xEF: {
			g_io_ports::writeShort((uint16_t) ctx->defaultFrame.edx, (uint16_t) ctx->defaultFrame.eax);
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
			uint8_t res = g_io_ports::readByte((uint16_t) ctx->defaultFrame.edx);
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
			uint16_t res = g_io_ports::readShort((uint16_t) ctx->defaultFrame.edx);
			ctx->defaultFrame.eax &= ~(0xFFFF);
			ctx->defaultFrame.eax |= res;
			++ctx->defaultFrame.eip;
			return VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL;
		}

			/**
			 * Unhandled operation
			 */
		default: {
			g_log_warn("%! unhandled opcode %h at linear location %h", "vm86", (uint32_t ) ip[0], ip);
			return VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE;
		}
		}
	}

	// Not reached
	return VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE;
}
