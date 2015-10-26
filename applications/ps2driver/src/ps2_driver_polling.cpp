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

#include "ps2_driver.hpp"
#include <ghost.h>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/tasking/lock.hpp>
#include <ghostuser/utils/utils.hpp>
#include <ghostuser/utils/logger.hpp>

#if DRIVER_OPERATION_MODE == DRIVER_OPERATION_MODE_POLLING

/**
 *
 */
void register_operation_mode() {
	g_logger::log("starting poll thread");
	g_create_thread((void*) polling_thread);
}

/**
 *
 */
void polling_thread() {
	while (true) {
		poll();
	}
}

/**
 *
 */
void poll() {

	static g_lock pollLock;
	pollLock.lock();

	uint8_t status;
	while (((status = g_utils::inportByte(0x64)) & 1) != 0) {
		uint8_t value = g_utils::inportByte(0x60);

		bool fromKeyboard = ((status & (1 << 5)) == 0);

		if (fromKeyboard) {
			handle_keyboard_data(value);
		} else {
			handle_mouse_data(value);
		}

		++packets_count;
	}

	pollLock.unlock();
}

#endif
