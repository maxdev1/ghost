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

#include "ghost/types.h"
#include "kernel/system/processor/processor.hpp"

void interruptsCheckPrerequisites();

void interruptsInitializeBsp();

void interruptsInitializeAp();

void interruptsEnable();

void interruptsDisable();

bool interruptsAreEnabled();

void interruptsInstallRoutines();

extern "C" g_virtual_address _interruptHandler(g_virtual_address state);

/**
 * @see assembly
 */
extern "C" void _irout0();
extern "C" void _irout1();
extern "C" void _irout2();
extern "C" void _irout3();
extern "C" void _irout4();
extern "C" void _irout5();
extern "C" void _irout6();
extern "C" void _irout7();
extern "C" void _irout8();
extern "C" void _irout9();
extern "C" void _irout10();
extern "C" void _irout11();
extern "C" void _irout12();
extern "C" void _irout13();
extern "C" void _irout14();
extern "C" void _irout15();
extern "C" void _irout16();
extern "C" void _irout17();
extern "C" void _irout18();
extern "C" void _irout19();
extern "C" void _irout20();
extern "C" void _irout21();
extern "C" void _irout22();
extern "C" void _irout23();
extern "C" void _irout24();
extern "C" void _irout25();
extern "C" void _irout26();
extern "C" void _irout27();
extern "C" void _irout28();
extern "C" void _irout29();
extern "C" void _irout30();
extern "C" void _irout31();

extern "C" void _ireq0();
extern "C" void _ireq1();
extern "C" void _ireq2();
extern "C" void _ireq3();
extern "C" void _ireq4();
extern "C" void _ireq5();
extern "C" void _ireq6();
extern "C" void _ireq7();
extern "C" void _ireq8();
extern "C" void _ireq9();
extern "C" void _ireq10();
extern "C" void _ireq11();
extern "C" void _ireq12();
extern "C" void _ireq13();
extern "C" void _ireq14();
extern "C" void _ireq15();
extern "C" void _ireq16();
extern "C" void _ireq17();
extern "C" void _ireq18();
extern "C" void _ireq19();
extern "C" void _ireq20();
extern "C" void _ireq21();
extern "C" void _ireq22();
extern "C" void _ireq23();
extern "C" void _ireq24();
extern "C" void _ireq25();
extern "C" void _ireq26();
extern "C" void _ireq27();
extern "C" void _ireq28();
extern "C" void _ireq29();
extern "C" void _ireq30();
extern "C" void _ireq31();
extern "C" void _ireq32();
extern "C" void _ireq33();
extern "C" void _ireq34();
extern "C" void _ireq35();
extern "C" void _ireq36();
extern "C" void _ireq37();
extern "C" void _ireq38();
extern "C" void _ireq39();
extern "C" void _ireq40();
extern "C" void _ireq41();
extern "C" void _ireq42();
extern "C" void _ireq43();
extern "C" void _ireq44();
extern "C" void _ireq45();
extern "C" void _ireq46();
extern "C" void _ireq47();
extern "C" void _ireq48();
extern "C" void _ireq49();
extern "C" void _ireq50();
extern "C" void _ireq51();
extern "C" void _ireq52();
extern "C" void _ireq53();
extern "C" void _ireq54();
extern "C" void _ireq55();
extern "C" void _ireq56();
extern "C" void _ireq57();
extern "C" void _ireq58();
extern "C" void _ireq59();
extern "C" void _ireq60();
extern "C" void _ireq61();
extern "C" void _ireq62();
extern "C" void _ireq63();
extern "C" void _ireq64();
extern "C" void _ireq65();
extern "C" void _ireq66();
extern "C" void _ireq67();
extern "C" void _ireq68();
extern "C" void _ireq69();
extern "C" void _ireq70();
extern "C" void _ireq71();
extern "C" void _ireq72();
extern "C" void _ireq73();
extern "C" void _ireq74();
extern "C" void _ireq75();
extern "C" void _ireq76();
extern "C" void _ireq77();
extern "C" void _ireq78();
extern "C" void _ireq79();
extern "C" void _ireq80();
extern "C" void _ireq81();
extern "C" void _ireq82();
extern "C" void _ireq83();
extern "C" void _ireq84();
extern "C" void _ireq85();
extern "C" void _ireq86();
extern "C" void _ireq87();
extern "C" void _ireq88();
extern "C" void _ireq89();
extern "C" void _ireq90();
extern "C" void _ireq91();
extern "C" void _ireq92();
extern "C" void _ireq93();
extern "C" void _ireq94();
extern "C" void _ireq95();
extern "C" void _ireqSyscall();
extern "C" void _ireq97();
extern "C" void _ireq98();
extern "C" void _ireq99();
extern "C" void _ireq100();
extern "C" void _ireq101();
extern "C" void _ireq102();
extern "C" void _ireq103();
extern "C" void _ireq104();
extern "C" void _ireq105();
extern "C" void _ireq106();
extern "C" void _ireq107();
extern "C" void _ireq108();
extern "C" void _ireq109();
extern "C" void _ireq110();
extern "C" void _ireq111();
extern "C" void _ireq112();
extern "C" void _ireq113();
extern "C" void _ireq114();
extern "C" void _ireq115();
extern "C" void _ireq116();
extern "C" void _ireq117();
extern "C" void _ireq118();
extern "C" void _ireq119();
extern "C" void _ireq120();
extern "C" void _ireq121();
extern "C" void _ireq122();
extern "C" void _ireq123();
extern "C" void _ireq124();
extern "C" void _ireq125();
extern "C" void _ireq126();
extern "C" void _ireq127();
extern "C" void _ireq128();
extern "C" void _ireq129();
extern "C" void _ireq130();
extern "C" void _ireq131();
extern "C" void _ireq132();
extern "C" void _ireq133();
extern "C" void _ireq134();
extern "C" void _ireq135();
extern "C" void _ireq136();
extern "C" void _ireq137();
extern "C" void _ireq138();
extern "C" void _ireq139();
extern "C" void _ireq140();
extern "C" void _ireq141();
extern "C" void _ireq142();
extern "C" void _ireq143();
extern "C" void _ireq144();
extern "C" void _ireq145();
extern "C" void _ireq146();
extern "C" void _ireq147();
extern "C" void _ireq148();
extern "C" void _ireq149();
extern "C" void _ireq150();
extern "C" void _ireq151();
extern "C" void _ireq152();
extern "C" void _ireq153();
extern "C" void _ireq154();
extern "C" void _ireq155();
extern "C" void _ireq156();
extern "C" void _ireq157();
extern "C" void _ireq158();
extern "C" void _ireq159();
extern "C" void _ireq160();
extern "C" void _ireq161();
extern "C" void _ireq162();
extern "C" void _ireq163();
extern "C" void _ireq164();
extern "C" void _ireq165();
extern "C" void _ireq166();
extern "C" void _ireq167();
extern "C" void _ireq168();
extern "C" void _ireq169();
extern "C" void _ireq170();
extern "C" void _ireq171();
extern "C" void _ireq172();
extern "C" void _ireq173();
extern "C" void _ireq174();
extern "C" void _ireq175();
extern "C" void _ireq176();
extern "C" void _ireq177();
extern "C" void _ireq178();
extern "C" void _ireq179();
extern "C" void _ireq180();
extern "C" void _ireq181();
extern "C" void _ireq182();
extern "C" void _ireq183();
extern "C" void _ireq184();
extern "C" void _ireq185();
extern "C" void _ireq186();
extern "C" void _ireq187();
extern "C" void _ireq188();
extern "C" void _ireq189();
extern "C" void _ireq190();
extern "C" void _ireq191();
extern "C" void _ireq192();
extern "C" void _ireq193();
extern "C" void _ireq194();
extern "C" void _ireq195();
extern "C" void _ireq196();
extern "C" void _ireq197();
extern "C" void _ireq198();
extern "C" void _ireq199();
extern "C" void _ireq200();
extern "C" void _ireq201();
extern "C" void _ireq202();
extern "C" void _ireq203();
extern "C" void _ireq204();
extern "C" void _ireq205();
extern "C" void _ireq206();
extern "C" void _ireq207();
extern "C" void _ireq208();
extern "C" void _ireq209();
extern "C" void _ireq210();
extern "C" void _ireq211();
extern "C" void _ireq212();
extern "C" void _ireq213();
extern "C" void _ireq214();
extern "C" void _ireq215();
extern "C" void _ireq216();
extern "C" void _ireq217();
extern "C" void _ireq218();
extern "C" void _ireq219();
extern "C" void _ireq220();
extern "C" void _ireq221();
extern "C" void _ireq222();
extern "C" void _ireq223();

#endif
