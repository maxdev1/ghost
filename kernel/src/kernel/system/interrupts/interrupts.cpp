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

#include "kernel/system/interrupts/interrupts.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/calls/syscall.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/interrupts/exceptions.hpp"
#include "kernel/system/interrupts/idt.hpp"
#include "kernel/system/interrupts/pic.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"

void _interruptsSendEndOfInterrupt(uint8_t irq);

void interruptsInitializeBsp()
{
	idtInitialize();
	idtInitializeLocal();
	interruptsInstallRoutines();
	requestsInitialize();

	if(lapicIsAvailable())
	{
		if(!ioapicAreAvailable())
			panic("%! no I/O APIC controllers found", "system");

		picDisable();
		lapicInitialize();
		ioapicInitializeAll();
	}
	else
	{
		picRemapIrqs();
		pitStartTimer();
	}
}

void interruptsInitializeAp()
{
	idtInitializeLocal();
	lapicInitialize();
}

extern "C" volatile g_processor_state* _interruptHandler(volatile g_processor_state* state)
{
	g_task* task = taskingGetCurrentTask();
	if(task)
		taskingSaveState(task, (g_processor_state*) state);

	if(state->intr < 0x20) // Exception
	{
		exceptionsHandle(task);
	}
	else if(state->intr == 0x80) // Syscall
	{
		syscallHandle(task);
	}
	else if(state->intr == 0x81) // Yield
	{
		taskingSchedule();
	}
	else if(state->intr == 0x82) // Privilege downgrade for spawn
	{
		taskingFinalizeSpawn(task);
	}
	else
	{
		uint8_t irq = state->intr - 0x20;
		if(irq == 0) // Timer
		{
			clockUpdate();
			taskingSchedule(true);
		}
		else
		{
			requestsHandle(task, irq);
		}
		_interruptsSendEndOfInterrupt(irq);
	}

	auto newTask = taskingGetCurrentTask();
	if(!newTask || !newTask->state)
		panic("%! attempted to switch to null task (%x) or state (%x)", "system", newTask, newTask->state);
	if(newTask != task)
		taskingRestoreState(newTask);

	// TODO currently we still fail when returning to the task
	logInfo("will return to task %i, rip %x", newTask->id, newTask->state->rip);

	return newTask->state;
}

void interruptsEnable()
{
	asm volatile("sti");
}

void interruptsDisable()
{
	asm volatile("cli");
}

bool interruptsAreEnabled()
{
	uint32_t eflags = processorReadEflags();
	return eflags & (1 << 9);
}

void _interruptsSendEndOfInterrupt(uint8_t irq)
{
	if(lapicIsAvailable())
		lapicSendEndOfInterrupt();
	else
		picSendEndOfInterrupt(irq);
}

void interruptsInstallRoutines()
{
	idtCreateGate(0x00, (void*) _isr00, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x01, (void*) _isr01, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x02, (void*) _isr02, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x03, (void*) _isr03, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x04, (void*) _isr04, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x05, (void*) _isr05, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x06, (void*) _isr06, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x07, (void*) _isr07, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x08, (void*) _isr08, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x09, (void*) _isr09, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0A, (void*) _isr0A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0B, (void*) _isr0B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0C, (void*) _isr0C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0D, (void*) _isr0D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0E, (void*) _isr0E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x0F, (void*) _isr0F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x10, (void*) _isr10, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x11, (void*) _isr11, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x12, (void*) _isr12, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x13, (void*) _isr13, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x14, (void*) _isr14, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x15, (void*) _isr15, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x16, (void*) _isr16, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x17, (void*) _isr17, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x18, (void*) _isr18, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x19, (void*) _isr19, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1A, (void*) _isr1A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1B, (void*) _isr1B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1C, (void*) _isr1C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1D, (void*) _isr1D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1E, (void*) _isr1E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x1F, (void*) _isr1F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x20, (void*) _isr20, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x21, (void*) _isr21, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x22, (void*) _isr22, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x23, (void*) _isr23, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x24, (void*) _isr24, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x25, (void*) _isr25, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x26, (void*) _isr26, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x27, (void*) _isr27, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x28, (void*) _isr28, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x29, (void*) _isr29, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2A, (void*) _isr2A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2B, (void*) _isr2B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2C, (void*) _isr2C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2D, (void*) _isr2D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2E, (void*) _isr2E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x2F, (void*) _isr2F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x30, (void*) _isr30, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x31, (void*) _isr31, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x32, (void*) _isr32, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x33, (void*) _isr33, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x34, (void*) _isr34, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x35, (void*) _isr35, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x36, (void*) _isr36, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x37, (void*) _isr37, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x38, (void*) _isr38, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x39, (void*) _isr39, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3A, (void*) _isr3A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3B, (void*) _isr3B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3C, (void*) _isr3C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3D, (void*) _isr3D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3E, (void*) _isr3E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x3F, (void*) _isr3F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x40, (void*) _isr40, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x41, (void*) _isr41, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x42, (void*) _isr42, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x43, (void*) _isr43, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x44, (void*) _isr44, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x45, (void*) _isr45, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x46, (void*) _isr46, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x47, (void*) _isr47, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x48, (void*) _isr48, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x49, (void*) _isr49, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4A, (void*) _isr4A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4B, (void*) _isr4B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4C, (void*) _isr4C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4D, (void*) _isr4D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4E, (void*) _isr4E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x4F, (void*) _isr4F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x50, (void*) _isr50, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x51, (void*) _isr51, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x52, (void*) _isr52, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x53, (void*) _isr53, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x54, (void*) _isr54, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x55, (void*) _isr55, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x56, (void*) _isr56, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x57, (void*) _isr57, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x58, (void*) _isr58, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x59, (void*) _isr59, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5A, (void*) _isr5A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5B, (void*) _isr5B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5C, (void*) _isr5C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5D, (void*) _isr5D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5E, (void*) _isr5E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x5F, (void*) _isr5F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x60, (void*) _isr60, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x61, (void*) _isr61, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x62, (void*) _isr62, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x63, (void*) _isr63, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x64, (void*) _isr64, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x65, (void*) _isr65, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x66, (void*) _isr66, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x67, (void*) _isr67, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x68, (void*) _isr68, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x69, (void*) _isr69, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6A, (void*) _isr6A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6B, (void*) _isr6B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6C, (void*) _isr6C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6D, (void*) _isr6D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6E, (void*) _isr6E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x6F, (void*) _isr6F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x70, (void*) _isr70, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x71, (void*) _isr71, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x72, (void*) _isr72, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x73, (void*) _isr73, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x74, (void*) _isr74, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x75, (void*) _isr75, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x76, (void*) _isr76, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x77, (void*) _isr77, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x78, (void*) _isr78, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x79, (void*) _isr79, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7A, (void*) _isr7A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7B, (void*) _isr7B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7C, (void*) _isr7C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7D, (void*) _isr7D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7E, (void*) _isr7E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x7F, (void*) _isr7F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x80, (void*) _isr80, G_IDT_FLAGS_INTERRUPT_GATE_USER); // syscall
	idtCreateGate(0x81, (void*) _isr81, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL); // yield
	idtCreateGate(0x82, (void*) _isr82, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL); // privilege downgrade
	idtCreateGate(0x83, (void*) _isr83, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x84, (void*) _isr84, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x85, (void*) _isr85, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x86, (void*) _isr86, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x87, (void*) _isr87, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x88, (void*) _isr88, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x89, (void*) _isr89, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8A, (void*) _isr8A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8B, (void*) _isr8B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8C, (void*) _isr8C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8D, (void*) _isr8D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8E, (void*) _isr8E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x8F, (void*) _isr8F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x90, (void*) _isr90, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x91, (void*) _isr91, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x92, (void*) _isr92, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x93, (void*) _isr93, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x94, (void*) _isr94, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x95, (void*) _isr95, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x96, (void*) _isr96, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x97, (void*) _isr97, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x98, (void*) _isr98, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x99, (void*) _isr99, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9A, (void*) _isr9A, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9B, (void*) _isr9B, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9C, (void*) _isr9C, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9D, (void*) _isr9D, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9E, (void*) _isr9E, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0x9F, (void*) _isr9F, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA0, (void*) _isrA0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA1, (void*) _isrA1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA2, (void*) _isrA2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA3, (void*) _isrA3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA4, (void*) _isrA4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA5, (void*) _isrA5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA6, (void*) _isrA6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA7, (void*) _isrA7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA8, (void*) _isrA8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xA9, (void*) _isrA9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAA, (void*) _isrAA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAB, (void*) _isrAB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAC, (void*) _isrAC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAD, (void*) _isrAD, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAE, (void*) _isrAE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xAF, (void*) _isrAF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB0, (void*) _isrB0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB1, (void*) _isrB1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB2, (void*) _isrB2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB3, (void*) _isrB3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB4, (void*) _isrB4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB5, (void*) _isrB5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB6, (void*) _isrB6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB7, (void*) _isrB7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB8, (void*) _isrB8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xB9, (void*) _isrB9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBA, (void*) _isrBA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBB, (void*) _isrBB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBC, (void*) _isrBC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBD, (void*) _isrBD, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBE, (void*) _isrBE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xBF, (void*) _isrBF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC0, (void*) _isrC0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC1, (void*) _isrC1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC2, (void*) _isrC2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC3, (void*) _isrC3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC4, (void*) _isrC4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC5, (void*) _isrC5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC6, (void*) _isrC6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC7, (void*) _isrC7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC8, (void*) _isrC8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xC9, (void*) _isrC9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCA, (void*) _isrCA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCB, (void*) _isrCB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCC, (void*) _isrCC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCD, (void*) _isrCD, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCE, (void*) _isrCE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xCF, (void*) _isrCF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD0, (void*) _isrD0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD1, (void*) _isrD1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD2, (void*) _isrD2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD3, (void*) _isrD3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD4, (void*) _isrD4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD5, (void*) _isrD5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD6, (void*) _isrD6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD7, (void*) _isrD7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD8, (void*) _isrD8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xD9, (void*) _isrD9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDA, (void*) _isrDA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDB, (void*) _isrDB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDC, (void*) _isrDC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDD, (void*) _isrDD, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDE, (void*) _isrDE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xDF, (void*) _isrDF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE0, (void*) _isrE0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE1, (void*) _isrE1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE2, (void*) _isrE2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE3, (void*) _isrE3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE4, (void*) _isrE4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE5, (void*) _isrE5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE6, (void*) _isrE6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE7, (void*) _isrE7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE8, (void*) _isrE8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xE9, (void*) _isrE9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xEA, (void*) _isrEA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xEB, (void*) _isrEB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xEC, (void*) _isrEC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xED, (void*) _isrED, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xEE, (void*) _isrEE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xEF, (void*) _isrEF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF0, (void*) _isrF0, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF1, (void*) _isrF1, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF2, (void*) _isrF2, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF3, (void*) _isrF3, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF4, (void*) _isrF4, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF5, (void*) _isrF5, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF6, (void*) _isrF6, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF7, (void*) _isrF7, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF8, (void*) _isrF8, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xF9, (void*) _isrF9, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFA, (void*) _isrFA, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFB, (void*) _isrFB, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFC, (void*) _isrFC, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFD, (void*) _isrFD, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFE, (void*) _isrFE, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
	idtCreateGate(0xFF, (void*) _isrFF, G_IDT_FLAGS_INTERRUPT_GATE_KERNEL);
}
