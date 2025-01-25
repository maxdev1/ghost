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

#ifndef __KERNEL_INTERRUPTS__
#define __KERNEL_INTERRUPTS__

#include "kernel/system/processor/processor.hpp"

/**
 * Pauses/resumes interrupts within the same scope.
 */
#define INTERRUPTS_PAUSE                        \
	int __intr_paused = interruptsAreEnabled(); \
	if(__intr_paused)                           \
		interruptsDisable();
#define INTERRUPTS_RESUME \
	if(__intr_paused)     \
		interruptsEnable();

/**
 * Sets up interrupts on the bootstrap processor.
 */
void interruptsInitializeBsp();

/**
 * Sets up interrupts on an application processor.
 */
void interruptsInitializeAp();

/**
 * Enables interrupts.
 */
void interruptsEnable();

/**
 * Disables interrupts.
 */
void interruptsDisable();

/**
 * @returns whether interrupts are enabled
 */
bool interruptsAreEnabled();

/**
 * Installs all ISRs into the IDT.
 */
void interruptsInstallRoutines();

/**
 * Interrupt handler routine. Stores the current task state and then processes the interrupt.
 */
extern "C" volatile g_processor_state* _interruptHandler(volatile g_processor_state* state);

/**
 * @see assembly
 */
extern "C" void _isr00();
extern "C" void _isr01();
extern "C" void _isr02();
extern "C" void _isr03();
extern "C" void _isr04();
extern "C" void _isr05();
extern "C" void _isr06();
extern "C" void _isr07();
extern "C" void _isr08();
extern "C" void _isr09();
extern "C" void _isr0A();
extern "C" void _isr0B();
extern "C" void _isr0C();
extern "C" void _isr0D();
extern "C" void _isr0E();
extern "C" void _isr0F();
extern "C" void _isr10();
extern "C" void _isr11();
extern "C" void _isr12();
extern "C" void _isr13();
extern "C" void _isr14();
extern "C" void _isr15();
extern "C" void _isr16();
extern "C" void _isr17();
extern "C" void _isr18();
extern "C" void _isr19();
extern "C" void _isr1A();
extern "C" void _isr1B();
extern "C" void _isr1C();
extern "C" void _isr1D();
extern "C" void _isr1E();
extern "C" void _isr1F();
extern "C" void _isr20();
extern "C" void _isr21();
extern "C" void _isr22();
extern "C" void _isr23();
extern "C" void _isr24();
extern "C" void _isr25();
extern "C" void _isr26();
extern "C" void _isr27();
extern "C" void _isr28();
extern "C" void _isr29();
extern "C" void _isr2A();
extern "C" void _isr2B();
extern "C" void _isr2C();
extern "C" void _isr2D();
extern "C" void _isr2E();
extern "C" void _isr2F();
extern "C" void _isr30();
extern "C" void _isr31();
extern "C" void _isr32();
extern "C" void _isr33();
extern "C" void _isr34();
extern "C" void _isr35();
extern "C" void _isr36();
extern "C" void _isr37();
extern "C" void _isr38();
extern "C" void _isr39();
extern "C" void _isr3A();
extern "C" void _isr3B();
extern "C" void _isr3C();
extern "C" void _isr3D();
extern "C" void _isr3E();
extern "C" void _isr3F();
extern "C" void _isr40();
extern "C" void _isr41();
extern "C" void _isr42();
extern "C" void _isr43();
extern "C" void _isr44();
extern "C" void _isr45();
extern "C" void _isr46();
extern "C" void _isr47();
extern "C" void _isr48();
extern "C" void _isr49();
extern "C" void _isr4A();
extern "C" void _isr4B();
extern "C" void _isr4C();
extern "C" void _isr4D();
extern "C" void _isr4E();
extern "C" void _isr4F();
extern "C" void _isr50();
extern "C" void _isr51();
extern "C" void _isr52();
extern "C" void _isr53();
extern "C" void _isr54();
extern "C" void _isr55();
extern "C" void _isr56();
extern "C" void _isr57();
extern "C" void _isr58();
extern "C" void _isr59();
extern "C" void _isr5A();
extern "C" void _isr5B();
extern "C" void _isr5C();
extern "C" void _isr5D();
extern "C" void _isr5E();
extern "C" void _isr5F();
extern "C" void _isr60();
extern "C" void _isr61();
extern "C" void _isr62();
extern "C" void _isr63();
extern "C" void _isr64();
extern "C" void _isr65();
extern "C" void _isr66();
extern "C" void _isr67();
extern "C" void _isr68();
extern "C" void _isr69();
extern "C" void _isr6A();
extern "C" void _isr6B();
extern "C" void _isr6C();
extern "C" void _isr6D();
extern "C" void _isr6E();
extern "C" void _isr6F();
extern "C" void _isr70();
extern "C" void _isr71();
extern "C" void _isr72();
extern "C" void _isr73();
extern "C" void _isr74();
extern "C" void _isr75();
extern "C" void _isr76();
extern "C" void _isr77();
extern "C" void _isr78();
extern "C" void _isr79();
extern "C" void _isr7A();
extern "C" void _isr7B();
extern "C" void _isr7C();
extern "C" void _isr7D();
extern "C" void _isr7E();
extern "C" void _isr7F();
extern "C" void _isr80();
extern "C" void _isr81();
extern "C" void _isr82();
extern "C" void _isr83();
extern "C" void _isr84();
extern "C" void _isr85();
extern "C" void _isr86();
extern "C" void _isr87();
extern "C" void _isr88();
extern "C" void _isr89();
extern "C" void _isr8A();
extern "C" void _isr8B();
extern "C" void _isr8C();
extern "C" void _isr8D();
extern "C" void _isr8E();
extern "C" void _isr8F();
extern "C" void _isr90();
extern "C" void _isr91();
extern "C" void _isr92();
extern "C" void _isr93();
extern "C" void _isr94();
extern "C" void _isr95();
extern "C" void _isr96();
extern "C" void _isr97();
extern "C" void _isr98();
extern "C" void _isr99();
extern "C" void _isr9A();
extern "C" void _isr9B();
extern "C" void _isr9C();
extern "C" void _isr9D();
extern "C" void _isr9E();
extern "C" void _isr9F();
extern "C" void _isrA0();
extern "C" void _isrA1();
extern "C" void _isrA2();
extern "C" void _isrA3();
extern "C" void _isrA4();
extern "C" void _isrA5();
extern "C" void _isrA6();
extern "C" void _isrA7();
extern "C" void _isrA8();
extern "C" void _isrA9();
extern "C" void _isrAA();
extern "C" void _isrAB();
extern "C" void _isrAC();
extern "C" void _isrAD();
extern "C" void _isrAE();
extern "C" void _isrAF();
extern "C" void _isrB0();
extern "C" void _isrB1();
extern "C" void _isrB2();
extern "C" void _isrB3();
extern "C" void _isrB4();
extern "C" void _isrB5();
extern "C" void _isrB6();
extern "C" void _isrB7();
extern "C" void _isrB8();
extern "C" void _isrB9();
extern "C" void _isrBA();
extern "C" void _isrBB();
extern "C" void _isrBC();
extern "C" void _isrBD();
extern "C" void _isrBE();
extern "C" void _isrBF();
extern "C" void _isrC0();
extern "C" void _isrC1();
extern "C" void _isrC2();
extern "C" void _isrC3();
extern "C" void _isrC4();
extern "C" void _isrC5();
extern "C" void _isrC6();
extern "C" void _isrC7();
extern "C" void _isrC8();
extern "C" void _isrC9();
extern "C" void _isrCA();
extern "C" void _isrCB();
extern "C" void _isrCC();
extern "C" void _isrCD();
extern "C" void _isrCE();
extern "C" void _isrCF();
extern "C" void _isrD0();
extern "C" void _isrD1();
extern "C" void _isrD2();
extern "C" void _isrD3();
extern "C" void _isrD4();
extern "C" void _isrD5();
extern "C" void _isrD6();
extern "C" void _isrD7();
extern "C" void _isrD8();
extern "C" void _isrD9();
extern "C" void _isrDA();
extern "C" void _isrDB();
extern "C" void _isrDC();
extern "C" void _isrDD();
extern "C" void _isrDE();
extern "C" void _isrDF();
extern "C" void _isrE0();
extern "C" void _isrE1();
extern "C" void _isrE2();
extern "C" void _isrE3();
extern "C" void _isrE4();
extern "C" void _isrE5();
extern "C" void _isrE6();
extern "C" void _isrE7();
extern "C" void _isrE8();
extern "C" void _isrE9();
extern "C" void _isrEA();
extern "C" void _isrEB();
extern "C" void _isrEC();
extern "C" void _isrED();
extern "C" void _isrEE();
extern "C" void _isrEF();
extern "C" void _isrF0();
extern "C" void _isrF1();
extern "C" void _isrF2();
extern "C" void _isrF3();
extern "C" void _isrF4();
extern "C" void _isrF5();
extern "C" void _isrF6();
extern "C" void _isrF7();
extern "C" void _isrF8();
extern "C" void _isrF9();
extern "C" void _isrFA();
extern "C" void _isrFB();
extern "C" void _isrFC();
extern "C" void _isrFD();
extern "C" void _isrFE();
extern "C" void _isrFF();

#endif
