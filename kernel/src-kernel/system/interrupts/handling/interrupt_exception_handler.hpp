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

#ifndef GHOST_INTERRUPTS_EXCEPTION_HANDLER
#define GHOST_INTERRUPTS_EXCEPTION_HANDLER

#include <system/processor_state.hpp>

/**
 *
 */
class g_interrupt_exception_handler {
public:
	static g_processor_state* handle(g_processor_state* cpuState);

	static g_processor_state* handleGeneralProtectionFault(g_processor_state* cpuState);
	static g_processor_state* handlePageFault(g_processor_state* cpuState);
	static g_processor_state* handleDivideError(g_processor_state* cpuState);
	static g_processor_state* handleInvalidOperationCode(g_processor_state* cpuState);

	static uint32_t getCR2();
	static void dump(g_processor_state* cpuState);
	static void printStackTrace(g_processor_state* cpuState);
};

#endif
