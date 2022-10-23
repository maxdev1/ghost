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
#include "kernel/calls/syscall.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/system/configuration.hpp"
#include "kernel/system/interrupts/apic/ioapic.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"
#include "kernel/system/interrupts/exceptions.hpp"
#include "kernel/system/interrupts/idt.hpp"
#include "kernel/system/interrupts/pic.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/timing/pit.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/tasking/tasking_state.hpp"
#include "kernel/utils/wait_queue.hpp"
#include "shared/panic.hpp"
#include "shared/system/mutex.hpp"

void _interruptsSendEndOfInterrupt(uint8_t irq);

void interruptsInitializeBsp()
{
	idtPrepare();
	idtLoad();
	interruptsInstallRoutines();
	requestsInitialize();

	if(lapicIsAvailable())
	{
		if(!ioapicAreAvailable())
			panic("%! no I/O APIC controllers found", "system");

		picDisable();
		lapicInitialize();
		ioapicInitializeAll();

		// Redirect keyboard (1) and mouse (12) IRQs
		ioapicCreateIsaRedirectionEntry(1, 1, 0);
		ioapicCreateIsaRedirectionEntry(12, 12, 0);
	}
	else
	{
		picRemapIrqs();
		pitStartTimer();
	}
}

void interruptsInitializeAp()
{
	idtLoad();
	lapicInitialize();
}

extern "C" volatile g_processor_state* _interruptHandler(volatile g_processor_state* state)
{
	g_task* task = taskingGetCurrentTask();

	if(state->intr == 0x82) // Privilege downgrade for spawn
	{
		// Prepare state to match the expected security level
		auto process = task->process;
		task->securityLevel = process->spawnArgs->securityLevel;
		taskingStateReset(task, process->spawnArgs->entry, task->securityLevel);
		waitQueueWake(&process->waitersSpawn);

		// Task is now prepared but waits until the spawner wakes it
		task->status = G_THREAD_STATUS_WAITING;
		taskingSchedule();
	}
	else
	{
		// In all usual cases, store the tasks state
		if(task)
		{
			task->state = (g_processor_state*) state;
			task->active = false;
		}
		else
		{
			taskingSchedule();
		}

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
		else
		{
			uint8_t irq = state->intr - 0x20;
			if(irq == 0) // Timer
			{
				clockUpdate();
				taskingSchedule();
			}
			else
			{
				requestsWriteToIrqDevice(task, irq);
			}

			_interruptsSendEndOfInterrupt(irq);
		}
	}

	return taskingGetCurrentTask()->state;
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
	idtCreateGate(0, (uint32_t) _irout0, 0x08, 0x8E);
	idtCreateGate(1, (uint32_t) _irout1, 0x08, 0x8E);
	idtCreateGate(2, (uint32_t) _irout2, 0x08, 0x8E);
	idtCreateGate(3, (uint32_t) _irout3, 0x08, 0x8E);
	idtCreateGate(4, (uint32_t) _irout4, 0x08, 0x8E);
	idtCreateGate(5, (uint32_t) _irout5, 0x08, 0x8E);
	idtCreateGate(6, (uint32_t) _irout6, 0x08, 0x8E);
	idtCreateGate(7, (uint32_t) _irout7, 0x08, 0x8E);
	idtCreateGate(8, (uint32_t) _irout8, 0x08, 0x8E);
	idtCreateGate(9, (uint32_t) _irout9, 0x08, 0x8E);
	idtCreateGate(10, (uint32_t) _irout10, 0x08, 0x8E);
	idtCreateGate(11, (uint32_t) _irout11, 0x08, 0x8E);
	idtCreateGate(12, (uint32_t) _irout12, 0x08, 0x8E);
	idtCreateGate(13, (uint32_t) _irout13, 0x08, 0x8E);
	idtCreateGate(14, (uint32_t) _irout14, 0x08, 0x8E);
	idtCreateGate(15, (uint32_t) _irout15, 0x08, 0x8E);
	idtCreateGate(16, (uint32_t) _irout16, 0x08, 0x8E);
	idtCreateGate(17, (uint32_t) _irout17, 0x08, 0x8E);
	idtCreateGate(18, (uint32_t) _irout18, 0x08, 0x8E);
	idtCreateGate(19, (uint32_t) _irout19, 0x08, 0x8E);
	idtCreateGate(20, (uint32_t) _irout20, 0x08, 0x8E);
	idtCreateGate(21, (uint32_t) _irout21, 0x08, 0x8E);
	idtCreateGate(22, (uint32_t) _irout22, 0x08, 0x8E);
	idtCreateGate(23, (uint32_t) _irout23, 0x08, 0x8E);
	idtCreateGate(24, (uint32_t) _irout24, 0x08, 0x8E);
	idtCreateGate(25, (uint32_t) _irout25, 0x08, 0x8E);
	idtCreateGate(26, (uint32_t) _irout26, 0x08, 0x8E);
	idtCreateGate(27, (uint32_t) _irout27, 0x08, 0x8E);
	idtCreateGate(28, (uint32_t) _irout28, 0x08, 0x8E);
	idtCreateGate(29, (uint32_t) _irout29, 0x08, 0x8E);
	idtCreateGate(30, (uint32_t) _irout30, 0x08, 0x8E);
	idtCreateGate(31, (uint32_t) _irout31, 0x08, 0x8E);

	idtCreateGate(32, (uint32_t) _ireq0, 0x08, 0x8E);
	idtCreateGate(33, (uint32_t) _ireq1, 0x08, 0x8E);
	idtCreateGate(34, (uint32_t) _ireq2, 0x08, 0x8E);
	idtCreateGate(35, (uint32_t) _ireq3, 0x08, 0x8E);
	idtCreateGate(36, (uint32_t) _ireq4, 0x08, 0x8E);
	idtCreateGate(37, (uint32_t) _ireq5, 0x08, 0x8E);
	idtCreateGate(38, (uint32_t) _ireq6, 0x08, 0x8E);
	idtCreateGate(39, (uint32_t) _ireq7, 0x08, 0x8E);
	idtCreateGate(40, (uint32_t) _ireq8, 0x08, 0x8E);
	idtCreateGate(41, (uint32_t) _ireq9, 0x08, 0x8E);
	idtCreateGate(42, (uint32_t) _ireq10, 0x08, 0x8E);
	idtCreateGate(43, (uint32_t) _ireq11, 0x08, 0x8E);
	idtCreateGate(44, (uint32_t) _ireq12, 0x08, 0x8E);
	idtCreateGate(45, (uint32_t) _ireq13, 0x08, 0x8E);
	idtCreateGate(46, (uint32_t) _ireq14, 0x08, 0x8E);
	idtCreateGate(47, (uint32_t) _ireq15, 0x08, 0x8E);
	idtCreateGate(48, (uint32_t) _ireq16, 0x08, 0x8E);
	idtCreateGate(49, (uint32_t) _ireq17, 0x08, 0x8E);
	idtCreateGate(50, (uint32_t) _ireq18, 0x08, 0x8E);
	idtCreateGate(51, (uint32_t) _ireq19, 0x08, 0x8E);
	idtCreateGate(52, (uint32_t) _ireq20, 0x08, 0x8E);
	idtCreateGate(53, (uint32_t) _ireq21, 0x08, 0x8E);
	idtCreateGate(54, (uint32_t) _ireq22, 0x08, 0x8E);
	idtCreateGate(55, (uint32_t) _ireq23, 0x08, 0x8E);
	idtCreateGate(56, (uint32_t) _ireq24, 0x08, 0x8E);
	idtCreateGate(57, (uint32_t) _ireq25, 0x08, 0x8E);
	idtCreateGate(58, (uint32_t) _ireq26, 0x08, 0x8E);
	idtCreateGate(59, (uint32_t) _ireq27, 0x08, 0x8E);
	idtCreateGate(60, (uint32_t) _ireq28, 0x08, 0x8E);
	idtCreateGate(61, (uint32_t) _ireq29, 0x08, 0x8E);
	idtCreateGate(62, (uint32_t) _ireq30, 0x08, 0x8E);
	idtCreateGate(63, (uint32_t) _ireq31, 0x08, 0x8E);
	idtCreateGate(64, (uint32_t) _ireq32, 0x08, 0x8E);
	idtCreateGate(65, (uint32_t) _ireq33, 0x08, 0x8E);
	idtCreateGate(66, (uint32_t) _ireq34, 0x08, 0x8E);
	idtCreateGate(67, (uint32_t) _ireq35, 0x08, 0x8E);
	idtCreateGate(68, (uint32_t) _ireq36, 0x08, 0x8E);
	idtCreateGate(69, (uint32_t) _ireq37, 0x08, 0x8E);
	idtCreateGate(70, (uint32_t) _ireq38, 0x08, 0x8E);
	idtCreateGate(71, (uint32_t) _ireq39, 0x08, 0x8E);
	idtCreateGate(72, (uint32_t) _ireq40, 0x08, 0x8E);
	idtCreateGate(73, (uint32_t) _ireq41, 0x08, 0x8E);
	idtCreateGate(74, (uint32_t) _ireq42, 0x08, 0x8E);
	idtCreateGate(75, (uint32_t) _ireq43, 0x08, 0x8E);
	idtCreateGate(76, (uint32_t) _ireq44, 0x08, 0x8E);
	idtCreateGate(77, (uint32_t) _ireq45, 0x08, 0x8E);
	idtCreateGate(78, (uint32_t) _ireq46, 0x08, 0x8E);
	idtCreateGate(79, (uint32_t) _ireq47, 0x08, 0x8E);
	idtCreateGate(80, (uint32_t) _ireq48, 0x08, 0x8E);
	idtCreateGate(81, (uint32_t) _ireq49, 0x08, 0x8E);
	idtCreateGate(82, (uint32_t) _ireq50, 0x08, 0x8E);
	idtCreateGate(83, (uint32_t) _ireq51, 0x08, 0x8E);
	idtCreateGate(84, (uint32_t) _ireq52, 0x08, 0x8E);
	idtCreateGate(85, (uint32_t) _ireq53, 0x08, 0x8E);
	idtCreateGate(86, (uint32_t) _ireq54, 0x08, 0x8E);
	idtCreateGate(87, (uint32_t) _ireq55, 0x08, 0x8E);
	idtCreateGate(88, (uint32_t) _ireq56, 0x08, 0x8E);
	idtCreateGate(89, (uint32_t) _ireq57, 0x08, 0x8E);
	idtCreateGate(90, (uint32_t) _ireq58, 0x08, 0x8E);
	idtCreateGate(91, (uint32_t) _ireq59, 0x08, 0x8E);
	idtCreateGate(92, (uint32_t) _ireq60, 0x08, 0x8E);
	idtCreateGate(93, (uint32_t) _ireq61, 0x08, 0x8E);
	idtCreateGate(94, (uint32_t) _ireq62, 0x08, 0x8E);
	idtCreateGate(95, (uint32_t) _ireq63, 0x08, 0x8E);
	idtCreateGate(96, (uint32_t) _ireq64, 0x08, 0x8E);
	idtCreateGate(97, (uint32_t) _ireq65, 0x08, 0x8E);
	idtCreateGate(98, (uint32_t) _ireq66, 0x08, 0x8E);
	idtCreateGate(99, (uint32_t) _ireq67, 0x08, 0x8E);
	idtCreateGate(100, (uint32_t) _ireq68, 0x08, 0x8E);
	idtCreateGate(101, (uint32_t) _ireq69, 0x08, 0x8E);
	idtCreateGate(102, (uint32_t) _ireq70, 0x08, 0x8E);
	idtCreateGate(103, (uint32_t) _ireq71, 0x08, 0x8E);
	idtCreateGate(104, (uint32_t) _ireq72, 0x08, 0x8E);
	idtCreateGate(105, (uint32_t) _ireq73, 0x08, 0x8E);
	idtCreateGate(106, (uint32_t) _ireq74, 0x08, 0x8E);
	idtCreateGate(107, (uint32_t) _ireq75, 0x08, 0x8E);
	idtCreateGate(108, (uint32_t) _ireq76, 0x08, 0x8E);
	idtCreateGate(109, (uint32_t) _ireq77, 0x08, 0x8E);
	idtCreateGate(110, (uint32_t) _ireq78, 0x08, 0x8E);
	idtCreateGate(111, (uint32_t) _ireq79, 0x08, 0x8E);
	idtCreateGate(112, (uint32_t) _ireq80, 0x08, 0x8E);
	idtCreateGate(113, (uint32_t) _ireq81, 0x08, 0x8E);
	idtCreateGate(114, (uint32_t) _ireq82, 0x08, 0x8E);
	idtCreateGate(115, (uint32_t) _ireq83, 0x08, 0x8E);
	idtCreateGate(116, (uint32_t) _ireq84, 0x08, 0x8E);
	idtCreateGate(117, (uint32_t) _ireq85, 0x08, 0x8E);
	idtCreateGate(118, (uint32_t) _ireq86, 0x08, 0x8E);
	idtCreateGate(119, (uint32_t) _ireq87, 0x08, 0x8E);
	idtCreateGate(120, (uint32_t) _ireq88, 0x08, 0x8E);
	idtCreateGate(121, (uint32_t) _ireq89, 0x08, 0x8E);
	idtCreateGate(122, (uint32_t) _ireq90, 0x08, 0x8E);
	idtCreateGate(123, (uint32_t) _ireq91, 0x08, 0x8E);
	idtCreateGate(124, (uint32_t) _ireq92, 0x08, 0x8E);
	idtCreateGate(125, (uint32_t) _ireq93, 0x08, 0x8E);
	idtCreateGate(126, (uint32_t) _ireq94, 0x08, 0x8E);
	idtCreateGate(127, (uint32_t) _ireq95, 0x08, 0x8E);
	idtCreateGate(128, (uint32_t) _ireqSyscall, 0x08, 0xEE);
	idtCreateGate(129, (uint32_t) _ireqYield, 0x08, 0x8E);
	idtCreateGate(130, (uint32_t) _ireq98, 0x08, 0x8E);
	idtCreateGate(131, (uint32_t) _ireq99, 0x08, 0x8E);
	idtCreateGate(132, (uint32_t) _ireq100, 0x08, 0x8E);
	idtCreateGate(133, (uint32_t) _ireq101, 0x08, 0x8E);
	idtCreateGate(134, (uint32_t) _ireq102, 0x08, 0x8E);
	idtCreateGate(135, (uint32_t) _ireq103, 0x08, 0x8E);
	idtCreateGate(136, (uint32_t) _ireq104, 0x08, 0x8E);
	idtCreateGate(137, (uint32_t) _ireq105, 0x08, 0x8E);
	idtCreateGate(138, (uint32_t) _ireq106, 0x08, 0x8E);
	idtCreateGate(139, (uint32_t) _ireq107, 0x08, 0x8E);
	idtCreateGate(140, (uint32_t) _ireq108, 0x08, 0x8E);
	idtCreateGate(141, (uint32_t) _ireq109, 0x08, 0x8E);
	idtCreateGate(142, (uint32_t) _ireq110, 0x08, 0x8E);
	idtCreateGate(143, (uint32_t) _ireq111, 0x08, 0x8E);
	idtCreateGate(144, (uint32_t) _ireq112, 0x08, 0x8E);
	idtCreateGate(145, (uint32_t) _ireq113, 0x08, 0x8E);
	idtCreateGate(146, (uint32_t) _ireq114, 0x08, 0x8E);
	idtCreateGate(147, (uint32_t) _ireq115, 0x08, 0x8E);
	idtCreateGate(148, (uint32_t) _ireq116, 0x08, 0x8E);
	idtCreateGate(149, (uint32_t) _ireq117, 0x08, 0x8E);
	idtCreateGate(150, (uint32_t) _ireq118, 0x08, 0x8E);
	idtCreateGate(151, (uint32_t) _ireq119, 0x08, 0x8E);
	idtCreateGate(152, (uint32_t) _ireq120, 0x08, 0x8E);
	idtCreateGate(153, (uint32_t) _ireq121, 0x08, 0x8E);
	idtCreateGate(154, (uint32_t) _ireq122, 0x08, 0x8E);
	idtCreateGate(155, (uint32_t) _ireq123, 0x08, 0x8E);
	idtCreateGate(156, (uint32_t) _ireq124, 0x08, 0x8E);
	idtCreateGate(157, (uint32_t) _ireq125, 0x08, 0x8E);
	idtCreateGate(158, (uint32_t) _ireq126, 0x08, 0x8E);
	idtCreateGate(159, (uint32_t) _ireq127, 0x08, 0x8E);
	idtCreateGate(160, (uint32_t) _ireq128, 0x08, 0x8E);
	idtCreateGate(161, (uint32_t) _ireq129, 0x08, 0x8E);
	idtCreateGate(162, (uint32_t) _ireq130, 0x08, 0x8E);
	idtCreateGate(163, (uint32_t) _ireq131, 0x08, 0x8E);
	idtCreateGate(164, (uint32_t) _ireq132, 0x08, 0x8E);
	idtCreateGate(165, (uint32_t) _ireq133, 0x08, 0x8E);
	idtCreateGate(166, (uint32_t) _ireq134, 0x08, 0x8E);
	idtCreateGate(167, (uint32_t) _ireq135, 0x08, 0x8E);
	idtCreateGate(168, (uint32_t) _ireq136, 0x08, 0x8E);
	idtCreateGate(169, (uint32_t) _ireq137, 0x08, 0x8E);
	idtCreateGate(170, (uint32_t) _ireq138, 0x08, 0x8E);
	idtCreateGate(171, (uint32_t) _ireq139, 0x08, 0x8E);
	idtCreateGate(172, (uint32_t) _ireq140, 0x08, 0x8E);
	idtCreateGate(173, (uint32_t) _ireq141, 0x08, 0x8E);
	idtCreateGate(174, (uint32_t) _ireq142, 0x08, 0x8E);
	idtCreateGate(175, (uint32_t) _ireq143, 0x08, 0x8E);
	idtCreateGate(176, (uint32_t) _ireq144, 0x08, 0x8E);
	idtCreateGate(177, (uint32_t) _ireq145, 0x08, 0x8E);
	idtCreateGate(178, (uint32_t) _ireq146, 0x08, 0x8E);
	idtCreateGate(179, (uint32_t) _ireq147, 0x08, 0x8E);
	idtCreateGate(180, (uint32_t) _ireq148, 0x08, 0x8E);
	idtCreateGate(181, (uint32_t) _ireq149, 0x08, 0x8E);
	idtCreateGate(182, (uint32_t) _ireq150, 0x08, 0x8E);
	idtCreateGate(183, (uint32_t) _ireq151, 0x08, 0x8E);
	idtCreateGate(184, (uint32_t) _ireq152, 0x08, 0x8E);
	idtCreateGate(185, (uint32_t) _ireq153, 0x08, 0x8E);
	idtCreateGate(186, (uint32_t) _ireq154, 0x08, 0x8E);
	idtCreateGate(187, (uint32_t) _ireq155, 0x08, 0x8E);
	idtCreateGate(188, (uint32_t) _ireq156, 0x08, 0x8E);
	idtCreateGate(189, (uint32_t) _ireq157, 0x08, 0x8E);
	idtCreateGate(190, (uint32_t) _ireq158, 0x08, 0x8E);
	idtCreateGate(191, (uint32_t) _ireq159, 0x08, 0x8E);
	idtCreateGate(192, (uint32_t) _ireq160, 0x08, 0x8E);
	idtCreateGate(193, (uint32_t) _ireq161, 0x08, 0x8E);
	idtCreateGate(194, (uint32_t) _ireq162, 0x08, 0x8E);
	idtCreateGate(195, (uint32_t) _ireq163, 0x08, 0x8E);
	idtCreateGate(196, (uint32_t) _ireq164, 0x08, 0x8E);
	idtCreateGate(197, (uint32_t) _ireq165, 0x08, 0x8E);
	idtCreateGate(198, (uint32_t) _ireq166, 0x08, 0x8E);
	idtCreateGate(199, (uint32_t) _ireq167, 0x08, 0x8E);
	idtCreateGate(200, (uint32_t) _ireq168, 0x08, 0x8E);
	idtCreateGate(201, (uint32_t) _ireq169, 0x08, 0x8E);
	idtCreateGate(202, (uint32_t) _ireq170, 0x08, 0x8E);
	idtCreateGate(203, (uint32_t) _ireq171, 0x08, 0x8E);
	idtCreateGate(204, (uint32_t) _ireq172, 0x08, 0x8E);
	idtCreateGate(205, (uint32_t) _ireq173, 0x08, 0x8E);
	idtCreateGate(206, (uint32_t) _ireq174, 0x08, 0x8E);
	idtCreateGate(207, (uint32_t) _ireq175, 0x08, 0x8E);
	idtCreateGate(208, (uint32_t) _ireq176, 0x08, 0x8E);
	idtCreateGate(209, (uint32_t) _ireq177, 0x08, 0x8E);
	idtCreateGate(210, (uint32_t) _ireq178, 0x08, 0x8E);
	idtCreateGate(211, (uint32_t) _ireq179, 0x08, 0x8E);
	idtCreateGate(212, (uint32_t) _ireq180, 0x08, 0x8E);
	idtCreateGate(213, (uint32_t) _ireq181, 0x08, 0x8E);
	idtCreateGate(214, (uint32_t) _ireq182, 0x08, 0x8E);
	idtCreateGate(215, (uint32_t) _ireq183, 0x08, 0x8E);
	idtCreateGate(216, (uint32_t) _ireq184, 0x08, 0x8E);
	idtCreateGate(217, (uint32_t) _ireq185, 0x08, 0x8E);
	idtCreateGate(218, (uint32_t) _ireq186, 0x08, 0x8E);
	idtCreateGate(219, (uint32_t) _ireq187, 0x08, 0x8E);
	idtCreateGate(220, (uint32_t) _ireq188, 0x08, 0x8E);
	idtCreateGate(221, (uint32_t) _ireq189, 0x08, 0x8E);
	idtCreateGate(222, (uint32_t) _ireq190, 0x08, 0x8E);
	idtCreateGate(223, (uint32_t) _ireq191, 0x08, 0x8E);
	idtCreateGate(224, (uint32_t) _ireq192, 0x08, 0x8E);
	idtCreateGate(225, (uint32_t) _ireq193, 0x08, 0x8E);
	idtCreateGate(226, (uint32_t) _ireq194, 0x08, 0x8E);
	idtCreateGate(227, (uint32_t) _ireq195, 0x08, 0x8E);
	idtCreateGate(228, (uint32_t) _ireq196, 0x08, 0x8E);
	idtCreateGate(229, (uint32_t) _ireq197, 0x08, 0x8E);
	idtCreateGate(230, (uint32_t) _ireq198, 0x08, 0x8E);
	idtCreateGate(231, (uint32_t) _ireq199, 0x08, 0x8E);
	idtCreateGate(232, (uint32_t) _ireq200, 0x08, 0x8E);
	idtCreateGate(233, (uint32_t) _ireq201, 0x08, 0x8E);
	idtCreateGate(234, (uint32_t) _ireq202, 0x08, 0x8E);
	idtCreateGate(235, (uint32_t) _ireq203, 0x08, 0x8E);
	idtCreateGate(236, (uint32_t) _ireq204, 0x08, 0x8E);
	idtCreateGate(237, (uint32_t) _ireq205, 0x08, 0x8E);
	idtCreateGate(238, (uint32_t) _ireq206, 0x08, 0x8E);
	idtCreateGate(239, (uint32_t) _ireq207, 0x08, 0x8E);
	idtCreateGate(240, (uint32_t) _ireq208, 0x08, 0x8E);
	idtCreateGate(241, (uint32_t) _ireq209, 0x08, 0x8E);
	idtCreateGate(242, (uint32_t) _ireq210, 0x08, 0x8E);
	idtCreateGate(243, (uint32_t) _ireq211, 0x08, 0x8E);
	idtCreateGate(244, (uint32_t) _ireq212, 0x08, 0x8E);
	idtCreateGate(245, (uint32_t) _ireq213, 0x08, 0x8E);
	idtCreateGate(246, (uint32_t) _ireq214, 0x08, 0x8E);
	idtCreateGate(247, (uint32_t) _ireq215, 0x08, 0x8E);
	idtCreateGate(248, (uint32_t) _ireq216, 0x08, 0x8E);
	idtCreateGate(249, (uint32_t) _ireq217, 0x08, 0x8E);
	idtCreateGate(250, (uint32_t) _ireq218, 0x08, 0x8E);
	idtCreateGate(251, (uint32_t) _ireq219, 0x08, 0x8E);
	idtCreateGate(252, (uint32_t) _ireq220, 0x08, 0x8E);
	idtCreateGate(253, (uint32_t) _ireq221, 0x08, 0x8E);
	idtCreateGate(254, (uint32_t) _ireq222, 0x08, 0x8E);
	idtCreateGate(255, (uint32_t) _ireq223, 0x08, 0x8E);
}
