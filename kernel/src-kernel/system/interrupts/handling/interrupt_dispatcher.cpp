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

#include <system/interrupts/handling/interrupt_dispatcher.hpp>
#include <system/interrupts/handling/interrupt_stubs.hpp>
#include <system/interrupts/descriptors/idt.hpp>
#include <system/interrupts/pic.hpp>
#include <system/interrupts/lapic.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>
#include <system/interrupts/handling/interrupt_exception_dispatcher.hpp>
#include <system/interrupts/handling/interrupt_request_dispatcher.hpp>
#include <tasking/tasking.hpp>
#include <system/io_ports.hpp>
#include <system/processor_state.hpp>
#include <system/smp/global_lock.hpp>
#include <tasking/wait/waiter.hpp>

/**
 * Interrupt handler routine, called by the interrupt stubs (assembly file)
 *
 * @param cpuState	the current CPU state
 * @return the CPU state to be applied
 */
extern "C" g_processor_state* _interruptHandler(g_processor_state* cpuState) {
	return g_interrupt_dispatcher::handle(cpuState);
}

static g_global_lock __interrupt_global_lock;

/**
 * 
 */
g_processor_state* g_interrupt_dispatcher::handle(g_processor_state* cpuState) {

	// TODO avoid global lock, lock per operation
	__interrupt_global_lock.lock();

	// save current task state
	g_thread* current_thread = g_tasking::save(cpuState);

	/*
	 Exceptions (interrupts below 0x20) are redirected to the exception handler,
	 while requests are redirected to the request handler.
	 */
	if (cpuState->intr < 0x20) {
		current_thread = g_interrupt_exception_dispatcher::handle(current_thread);
	} else {
		current_thread = g_interrupt_request_dispatcher::handle(current_thread);
	}

	// sanity check
	if (current_thread->waitManager != nullptr) {
		g_kernel::panic("scheduled thread %i had a wait manager ('%s')", current_thread->id, current_thread->waitManager->debug_name());
	}

	// send end of interrupt
	g_lapic::send_eoi();

	// TODO
	__interrupt_global_lock.unlock();

	return current_thread->cpuState;
}

/**
 * 
 */
void g_interrupt_dispatcher::install() {
	g_idt::createGate(0, (uint32_t) _irout0, 0x08, 0x8E);
	g_idt::createGate(1, (uint32_t) _irout1, 0x08, 0x8E);
	g_idt::createGate(2, (uint32_t) _irout2, 0x08, 0x8E);
	g_idt::createGate(3, (uint32_t) _irout3, 0x08, 0x8E);
	g_idt::createGate(4, (uint32_t) _irout4, 0x08, 0x8E);
	g_idt::createGate(5, (uint32_t) _irout5, 0x08, 0x8E);
	g_idt::createGate(6, (uint32_t) _irout6, 0x08, 0x8E);
	g_idt::createGate(7, (uint32_t) _irout7, 0x08, 0x8E);
	g_idt::createGate(8, (uint32_t) _irout8, 0x08, 0x8E);
	g_idt::createGate(9, (uint32_t) _irout9, 0x08, 0x8E);
	g_idt::createGate(10, (uint32_t) _irout10, 0x08, 0x8E);
	g_idt::createGate(11, (uint32_t) _irout11, 0x08, 0x8E);
	g_idt::createGate(12, (uint32_t) _irout12, 0x08, 0x8E);
	g_idt::createGate(13, (uint32_t) _irout13, 0x08, 0x8E);
	g_idt::createGate(14, (uint32_t) _irout14, 0x08, 0x8E);
	g_idt::createGate(15, (uint32_t) _irout15, 0x08, 0x8E);
	g_idt::createGate(16, (uint32_t) _irout16, 0x08, 0x8E);
	g_idt::createGate(17, (uint32_t) _irout17, 0x08, 0x8E);
	g_idt::createGate(18, (uint32_t) _irout18, 0x08, 0x8E);
	g_idt::createGate(19, (uint32_t) _irout19, 0x08, 0x8E);
	g_idt::createGate(20, (uint32_t) _irout20, 0x08, 0x8E);
	g_idt::createGate(21, (uint32_t) _irout21, 0x08, 0x8E);
	g_idt::createGate(22, (uint32_t) _irout22, 0x08, 0x8E);
	g_idt::createGate(23, (uint32_t) _irout23, 0x08, 0x8E);
	g_idt::createGate(24, (uint32_t) _irout24, 0x08, 0x8E);
	g_idt::createGate(25, (uint32_t) _irout25, 0x08, 0x8E);
	g_idt::createGate(26, (uint32_t) _irout26, 0x08, 0x8E);
	g_idt::createGate(27, (uint32_t) _irout27, 0x08, 0x8E);
	g_idt::createGate(28, (uint32_t) _irout28, 0x08, 0x8E);
	g_idt::createGate(29, (uint32_t) _irout29, 0x08, 0x8E);
	g_idt::createGate(30, (uint32_t) _irout30, 0x08, 0x8E);
	g_idt::createGate(31, (uint32_t) _irout31, 0x08, 0x8E);

	g_idt::createGate(32, (uint32_t) _ireq0, 0x08, 0x8E);
	g_idt::createGate(33, (uint32_t) _ireq1, 0x08, 0x8E);
	g_idt::createGate(34, (uint32_t) _ireq2, 0x08, 0x8E);
	g_idt::createGate(35, (uint32_t) _ireq3, 0x08, 0x8E);
	g_idt::createGate(36, (uint32_t) _ireq4, 0x08, 0x8E);
	g_idt::createGate(37, (uint32_t) _ireq5, 0x08, 0x8E);
	g_idt::createGate(38, (uint32_t) _ireq6, 0x08, 0x8E);
	g_idt::createGate(39, (uint32_t) _ireq7, 0x08, 0x8E);
	g_idt::createGate(40, (uint32_t) _ireq8, 0x08, 0x8E);
	g_idt::createGate(41, (uint32_t) _ireq9, 0x08, 0x8E);
	g_idt::createGate(42, (uint32_t) _ireq10, 0x08, 0x8E);
	g_idt::createGate(43, (uint32_t) _ireq11, 0x08, 0x8E);
	g_idt::createGate(44, (uint32_t) _ireq12, 0x08, 0x8E);
	g_idt::createGate(45, (uint32_t) _ireq13, 0x08, 0x8E);
	g_idt::createGate(46, (uint32_t) _ireq14, 0x08, 0x8E);
	g_idt::createGate(47, (uint32_t) _ireq15, 0x08, 0x8E);
	g_idt::createGate(48, (uint32_t) _ireq16, 0x08, 0x8E);
	g_idt::createGate(49, (uint32_t) _ireq17, 0x08, 0x8E);
	g_idt::createGate(50, (uint32_t) _ireq18, 0x08, 0x8E);
	g_idt::createGate(51, (uint32_t) _ireq19, 0x08, 0x8E);
	g_idt::createGate(52, (uint32_t) _ireq20, 0x08, 0x8E);
	g_idt::createGate(53, (uint32_t) _ireq21, 0x08, 0x8E);
	g_idt::createGate(54, (uint32_t) _ireq22, 0x08, 0x8E);
	g_idt::createGate(55, (uint32_t) _ireq23, 0x08, 0x8E);
	g_idt::createGate(56, (uint32_t) _ireq24, 0x08, 0x8E);
	g_idt::createGate(57, (uint32_t) _ireq25, 0x08, 0x8E);
	g_idt::createGate(58, (uint32_t) _ireq26, 0x08, 0x8E);
	g_idt::createGate(59, (uint32_t) _ireq27, 0x08, 0x8E);
	g_idt::createGate(60, (uint32_t) _ireq28, 0x08, 0x8E);
	g_idt::createGate(61, (uint32_t) _ireq29, 0x08, 0x8E);
	g_idt::createGate(62, (uint32_t) _ireq30, 0x08, 0x8E);
	g_idt::createGate(63, (uint32_t) _ireq31, 0x08, 0x8E);
	g_idt::createGate(64, (uint32_t) _ireq32, 0x08, 0x8E);
	g_idt::createGate(65, (uint32_t) _ireq33, 0x08, 0x8E);
	g_idt::createGate(66, (uint32_t) _ireq34, 0x08, 0x8E);
	g_idt::createGate(67, (uint32_t) _ireq35, 0x08, 0x8E);
	g_idt::createGate(68, (uint32_t) _ireq36, 0x08, 0x8E);
	g_idt::createGate(69, (uint32_t) _ireq37, 0x08, 0x8E);
	g_idt::createGate(70, (uint32_t) _ireq38, 0x08, 0x8E);
	g_idt::createGate(71, (uint32_t) _ireq39, 0x08, 0x8E);
	g_idt::createGate(72, (uint32_t) _ireq40, 0x08, 0x8E);
	g_idt::createGate(73, (uint32_t) _ireq41, 0x08, 0x8E);
	g_idt::createGate(74, (uint32_t) _ireq42, 0x08, 0x8E);
	g_idt::createGate(75, (uint32_t) _ireq43, 0x08, 0x8E);
	g_idt::createGate(76, (uint32_t) _ireq44, 0x08, 0x8E);
	g_idt::createGate(77, (uint32_t) _ireq45, 0x08, 0x8E);
	g_idt::createGate(78, (uint32_t) _ireq46, 0x08, 0x8E);
	g_idt::createGate(79, (uint32_t) _ireq47, 0x08, 0x8E);
	g_idt::createGate(80, (uint32_t) _ireq48, 0x08, 0x8E);
	g_idt::createGate(81, (uint32_t) _ireq49, 0x08, 0x8E);
	g_idt::createGate(82, (uint32_t) _ireq50, 0x08, 0x8E);
	g_idt::createGate(83, (uint32_t) _ireq51, 0x08, 0x8E);
	g_idt::createGate(84, (uint32_t) _ireq52, 0x08, 0x8E);
	g_idt::createGate(85, (uint32_t) _ireq53, 0x08, 0x8E);
	g_idt::createGate(86, (uint32_t) _ireq54, 0x08, 0x8E);
	g_idt::createGate(87, (uint32_t) _ireq55, 0x08, 0x8E);
	g_idt::createGate(88, (uint32_t) _ireq56, 0x08, 0x8E);
	g_idt::createGate(89, (uint32_t) _ireq57, 0x08, 0x8E);
	g_idt::createGate(90, (uint32_t) _ireq58, 0x08, 0x8E);
	g_idt::createGate(91, (uint32_t) _ireq59, 0x08, 0x8E);
	g_idt::createGate(92, (uint32_t) _ireq60, 0x08, 0x8E);
	g_idt::createGate(93, (uint32_t) _ireq61, 0x08, 0x8E);
	g_idt::createGate(94, (uint32_t) _ireq62, 0x08, 0x8E);
	g_idt::createGate(95, (uint32_t) _ireq63, 0x08, 0x8E);
	g_idt::createGate(96, (uint32_t) _ireq64, 0x08, 0x8E);
	g_idt::createGate(97, (uint32_t) _ireq65, 0x08, 0x8E);
	g_idt::createGate(98, (uint32_t) _ireq66, 0x08, 0x8E);
	g_idt::createGate(99, (uint32_t) _ireq67, 0x08, 0x8E);
	g_idt::createGate(100, (uint32_t) _ireq68, 0x08, 0x8E);
	g_idt::createGate(101, (uint32_t) _ireq69, 0x08, 0x8E);
	g_idt::createGate(102, (uint32_t) _ireq70, 0x08, 0x8E);
	g_idt::createGate(103, (uint32_t) _ireq71, 0x08, 0x8E);
	g_idt::createGate(104, (uint32_t) _ireq72, 0x08, 0x8E);
	g_idt::createGate(105, (uint32_t) _ireq73, 0x08, 0x8E);
	g_idt::createGate(106, (uint32_t) _ireq74, 0x08, 0x8E);
	g_idt::createGate(107, (uint32_t) _ireq75, 0x08, 0x8E);
	g_idt::createGate(108, (uint32_t) _ireq76, 0x08, 0x8E);
	g_idt::createGate(109, (uint32_t) _ireq77, 0x08, 0x8E);
	g_idt::createGate(110, (uint32_t) _ireq78, 0x08, 0x8E);
	g_idt::createGate(111, (uint32_t) _ireq79, 0x08, 0x8E);
	g_idt::createGate(112, (uint32_t) _ireq80, 0x08, 0x8E);
	g_idt::createGate(113, (uint32_t) _ireq81, 0x08, 0x8E);
	g_idt::createGate(114, (uint32_t) _ireq82, 0x08, 0x8E);
	g_idt::createGate(115, (uint32_t) _ireq83, 0x08, 0x8E);
	g_idt::createGate(116, (uint32_t) _ireq84, 0x08, 0x8E);
	g_idt::createGate(117, (uint32_t) _ireq85, 0x08, 0x8E);
	g_idt::createGate(118, (uint32_t) _ireq86, 0x08, 0x8E);
	g_idt::createGate(119, (uint32_t) _ireq87, 0x08, 0x8E);
	g_idt::createGate(120, (uint32_t) _ireq88, 0x08, 0x8E);
	g_idt::createGate(121, (uint32_t) _ireq89, 0x08, 0x8E);
	g_idt::createGate(122, (uint32_t) _ireq90, 0x08, 0x8E);
	g_idt::createGate(123, (uint32_t) _ireq91, 0x08, 0x8E);
	g_idt::createGate(124, (uint32_t) _ireq92, 0x08, 0x8E);
	g_idt::createGate(125, (uint32_t) _ireq93, 0x08, 0x8E);
	g_idt::createGate(126, (uint32_t) _ireq94, 0x08, 0x8E);
	g_idt::createGate(127, (uint32_t) _ireq95, 0x08, 0x8E);
	g_idt::createGate(128, (uint32_t) _ireqSyscall, 0x08, 0xEE);
	g_idt::createGate(129, (uint32_t) _ireq97, 0x08, 0x8E);
	g_idt::createGate(130, (uint32_t) _ireq98, 0x08, 0x8E);
	g_idt::createGate(131, (uint32_t) _ireq99, 0x08, 0x8E);
	g_idt::createGate(132, (uint32_t) _ireq100, 0x08, 0x8E);
	g_idt::createGate(133, (uint32_t) _ireq101, 0x08, 0x8E);
	g_idt::createGate(134, (uint32_t) _ireq102, 0x08, 0x8E);
	g_idt::createGate(135, (uint32_t) _ireq103, 0x08, 0x8E);
	g_idt::createGate(136, (uint32_t) _ireq104, 0x08, 0x8E);
	g_idt::createGate(137, (uint32_t) _ireq105, 0x08, 0x8E);
	g_idt::createGate(138, (uint32_t) _ireq106, 0x08, 0x8E);
	g_idt::createGate(139, (uint32_t) _ireq107, 0x08, 0x8E);
	g_idt::createGate(140, (uint32_t) _ireq108, 0x08, 0x8E);
	g_idt::createGate(141, (uint32_t) _ireq109, 0x08, 0x8E);
	g_idt::createGate(142, (uint32_t) _ireq110, 0x08, 0x8E);
	g_idt::createGate(143, (uint32_t) _ireq111, 0x08, 0x8E);
	g_idt::createGate(144, (uint32_t) _ireq112, 0x08, 0x8E);
	g_idt::createGate(145, (uint32_t) _ireq113, 0x08, 0x8E);
	g_idt::createGate(146, (uint32_t) _ireq114, 0x08, 0x8E);
	g_idt::createGate(147, (uint32_t) _ireq115, 0x08, 0x8E);
	g_idt::createGate(148, (uint32_t) _ireq116, 0x08, 0x8E);
	g_idt::createGate(149, (uint32_t) _ireq117, 0x08, 0x8E);
	g_idt::createGate(150, (uint32_t) _ireq118, 0x08, 0x8E);
	g_idt::createGate(151, (uint32_t) _ireq119, 0x08, 0x8E);
	g_idt::createGate(152, (uint32_t) _ireq120, 0x08, 0x8E);
	g_idt::createGate(153, (uint32_t) _ireq121, 0x08, 0x8E);
	g_idt::createGate(154, (uint32_t) _ireq122, 0x08, 0x8E);
	g_idt::createGate(155, (uint32_t) _ireq123, 0x08, 0x8E);
	g_idt::createGate(156, (uint32_t) _ireq124, 0x08, 0x8E);
	g_idt::createGate(157, (uint32_t) _ireq125, 0x08, 0x8E);
	g_idt::createGate(158, (uint32_t) _ireq126, 0x08, 0x8E);
	g_idt::createGate(159, (uint32_t) _ireq127, 0x08, 0x8E);
	g_idt::createGate(160, (uint32_t) _ireq128, 0x08, 0x8E);
	g_idt::createGate(161, (uint32_t) _ireq129, 0x08, 0x8E);
	g_idt::createGate(162, (uint32_t) _ireq130, 0x08, 0x8E);
	g_idt::createGate(163, (uint32_t) _ireq131, 0x08, 0x8E);
	g_idt::createGate(164, (uint32_t) _ireq132, 0x08, 0x8E);
	g_idt::createGate(165, (uint32_t) _ireq133, 0x08, 0x8E);
	g_idt::createGate(166, (uint32_t) _ireq134, 0x08, 0x8E);
	g_idt::createGate(167, (uint32_t) _ireq135, 0x08, 0x8E);
	g_idt::createGate(168, (uint32_t) _ireq136, 0x08, 0x8E);
	g_idt::createGate(169, (uint32_t) _ireq137, 0x08, 0x8E);
	g_idt::createGate(170, (uint32_t) _ireq138, 0x08, 0x8E);
	g_idt::createGate(171, (uint32_t) _ireq139, 0x08, 0x8E);
	g_idt::createGate(172, (uint32_t) _ireq140, 0x08, 0x8E);
	g_idt::createGate(173, (uint32_t) _ireq141, 0x08, 0x8E);
	g_idt::createGate(174, (uint32_t) _ireq142, 0x08, 0x8E);
	g_idt::createGate(175, (uint32_t) _ireq143, 0x08, 0x8E);
	g_idt::createGate(176, (uint32_t) _ireq144, 0x08, 0x8E);
	g_idt::createGate(177, (uint32_t) _ireq145, 0x08, 0x8E);
	g_idt::createGate(178, (uint32_t) _ireq146, 0x08, 0x8E);
	g_idt::createGate(179, (uint32_t) _ireq147, 0x08, 0x8E);
	g_idt::createGate(180, (uint32_t) _ireq148, 0x08, 0x8E);
	g_idt::createGate(181, (uint32_t) _ireq149, 0x08, 0x8E);
	g_idt::createGate(182, (uint32_t) _ireq150, 0x08, 0x8E);
	g_idt::createGate(183, (uint32_t) _ireq151, 0x08, 0x8E);
	g_idt::createGate(184, (uint32_t) _ireq152, 0x08, 0x8E);
	g_idt::createGate(185, (uint32_t) _ireq153, 0x08, 0x8E);
	g_idt::createGate(186, (uint32_t) _ireq154, 0x08, 0x8E);
	g_idt::createGate(187, (uint32_t) _ireq155, 0x08, 0x8E);
	g_idt::createGate(188, (uint32_t) _ireq156, 0x08, 0x8E);
	g_idt::createGate(189, (uint32_t) _ireq157, 0x08, 0x8E);
	g_idt::createGate(190, (uint32_t) _ireq158, 0x08, 0x8E);
	g_idt::createGate(191, (uint32_t) _ireq159, 0x08, 0x8E);
	g_idt::createGate(192, (uint32_t) _ireq160, 0x08, 0x8E);
	g_idt::createGate(193, (uint32_t) _ireq161, 0x08, 0x8E);
	g_idt::createGate(194, (uint32_t) _ireq162, 0x08, 0x8E);
	g_idt::createGate(195, (uint32_t) _ireq163, 0x08, 0x8E);
	g_idt::createGate(196, (uint32_t) _ireq164, 0x08, 0x8E);
	g_idt::createGate(197, (uint32_t) _ireq165, 0x08, 0x8E);
	g_idt::createGate(198, (uint32_t) _ireq166, 0x08, 0x8E);
	g_idt::createGate(199, (uint32_t) _ireq167, 0x08, 0x8E);
	g_idt::createGate(200, (uint32_t) _ireq168, 0x08, 0x8E);
	g_idt::createGate(201, (uint32_t) _ireq169, 0x08, 0x8E);
	g_idt::createGate(202, (uint32_t) _ireq170, 0x08, 0x8E);
	g_idt::createGate(203, (uint32_t) _ireq171, 0x08, 0x8E);
	g_idt::createGate(204, (uint32_t) _ireq172, 0x08, 0x8E);
	g_idt::createGate(205, (uint32_t) _ireq173, 0x08, 0x8E);
	g_idt::createGate(206, (uint32_t) _ireq174, 0x08, 0x8E);
	g_idt::createGate(207, (uint32_t) _ireq175, 0x08, 0x8E);
	g_idt::createGate(208, (uint32_t) _ireq176, 0x08, 0x8E);
	g_idt::createGate(209, (uint32_t) _ireq177, 0x08, 0x8E);
	g_idt::createGate(210, (uint32_t) _ireq178, 0x08, 0x8E);
	g_idt::createGate(211, (uint32_t) _ireq179, 0x08, 0x8E);
	g_idt::createGate(212, (uint32_t) _ireq180, 0x08, 0x8E);
	g_idt::createGate(213, (uint32_t) _ireq181, 0x08, 0x8E);
	g_idt::createGate(214, (uint32_t) _ireq182, 0x08, 0x8E);
	g_idt::createGate(215, (uint32_t) _ireq183, 0x08, 0x8E);
	g_idt::createGate(216, (uint32_t) _ireq184, 0x08, 0x8E);
	g_idt::createGate(217, (uint32_t) _ireq185, 0x08, 0x8E);
	g_idt::createGate(218, (uint32_t) _ireq186, 0x08, 0x8E);
	g_idt::createGate(219, (uint32_t) _ireq187, 0x08, 0x8E);
	g_idt::createGate(220, (uint32_t) _ireq188, 0x08, 0x8E);
	g_idt::createGate(221, (uint32_t) _ireq189, 0x08, 0x8E);
	g_idt::createGate(222, (uint32_t) _ireq190, 0x08, 0x8E);
	g_idt::createGate(223, (uint32_t) _ireq191, 0x08, 0x8E);
	g_idt::createGate(224, (uint32_t) _ireq192, 0x08, 0x8E);
	g_idt::createGate(225, (uint32_t) _ireq193, 0x08, 0x8E);
	g_idt::createGate(226, (uint32_t) _ireq194, 0x08, 0x8E);
	g_idt::createGate(227, (uint32_t) _ireq195, 0x08, 0x8E);
	g_idt::createGate(228, (uint32_t) _ireq196, 0x08, 0x8E);
	g_idt::createGate(229, (uint32_t) _ireq197, 0x08, 0x8E);
	g_idt::createGate(230, (uint32_t) _ireq198, 0x08, 0x8E);
	g_idt::createGate(231, (uint32_t) _ireq199, 0x08, 0x8E);
	g_idt::createGate(232, (uint32_t) _ireq200, 0x08, 0x8E);
	g_idt::createGate(233, (uint32_t) _ireq201, 0x08, 0x8E);
	g_idt::createGate(234, (uint32_t) _ireq202, 0x08, 0x8E);
	g_idt::createGate(235, (uint32_t) _ireq203, 0x08, 0x8E);
	g_idt::createGate(236, (uint32_t) _ireq204, 0x08, 0x8E);
	g_idt::createGate(237, (uint32_t) _ireq205, 0x08, 0x8E);
	g_idt::createGate(238, (uint32_t) _ireq206, 0x08, 0x8E);
	g_idt::createGate(239, (uint32_t) _ireq207, 0x08, 0x8E);
	g_idt::createGate(240, (uint32_t) _ireq208, 0x08, 0x8E);
	g_idt::createGate(241, (uint32_t) _ireq209, 0x08, 0x8E);
	g_idt::createGate(242, (uint32_t) _ireq210, 0x08, 0x8E);
	g_idt::createGate(243, (uint32_t) _ireq211, 0x08, 0x8E);
	g_idt::createGate(244, (uint32_t) _ireq212, 0x08, 0x8E);
	g_idt::createGate(245, (uint32_t) _ireq213, 0x08, 0x8E);
	g_idt::createGate(246, (uint32_t) _ireq214, 0x08, 0x8E);
	g_idt::createGate(247, (uint32_t) _ireq215, 0x08, 0x8E);
	g_idt::createGate(248, (uint32_t) _ireq216, 0x08, 0x8E);
	g_idt::createGate(249, (uint32_t) _ireq217, 0x08, 0x8E);
	g_idt::createGate(250, (uint32_t) _ireq218, 0x08, 0x8E);
	g_idt::createGate(251, (uint32_t) _ireq219, 0x08, 0x8E);
	g_idt::createGate(252, (uint32_t) _ireq220, 0x08, 0x8E);
	g_idt::createGate(253, (uint32_t) _ireq221, 0x08, 0x8E);
	g_idt::createGate(254, (uint32_t) _ireq222, 0x08, 0x8E);
	g_idt::createGate(255, (uint32_t) _ireq223, 0x08, 0x8E);
}
