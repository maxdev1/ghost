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

#ifndef __PS2_DRIVER__
#define __PS2_DRIVER__

#include <stdint.h>

/**
 * The operation mode declares whether the IRQs shall be handled by polling the kernel
 * or by registering a handler that is called upon interrupt receival.
 *
 * The polling mode might create higher latency due to the kernel being involved more,
 * but is more stable as the interrupt is handled on kernel side.
 *
 * Default operation mode is the IRQ-triggered mode.
 */
#define DRIVER_OPERATION_MODE_IRQ_TRIGGERED		0
#define DRIVER_OPERATION_MODE_POLLING			1

#define DRIVER_OPERATION_MODE					DRIVER_OPERATION_MODE_IRQ_TRIGGERED

/**
 * Operation mode specific header files
 */
#if DRIVER_OPERATION_MODE == DRIVER_OPERATION_MODE_POLLING
#include "ps2_driver_polling.hpp"
#elif DRIVER_OPERATION_MODE == DRIVER_OPERATION_MODE_IRQ_TRIGGERED
#include "ps2_driver_irq_triggered.hpp"
#endif

/**
 * Counter for the absolute number of packages that where received from the devices.
 */
extern uint64_t packets_count;

/**
 * Used when waiting for a buffer to determine whether the driver
 * shall wait for the input or output buffer of the device.
 */
enum ps2_buffer_t {
	PS2_OUT, PS2_IN
};

/**
 * Initializes the mouse by resetting it to the defaults and enabling it. This involves
 * a series of commands that are written to the device.
 */
void initialize_mouse();

/**
 * Waits for the PS2 input or output buffer by querying the flags in the status byte.
 */
void wait_for_buffer(ps2_buffer_t mode);

/**
 * Writes a byte to the mouse.
 *
 * @param b
 * 		byte to write
 */
void write_to_mouse(uint8_t b);

/**
 * Registers operation mode specific handling logic.
 */
void register_operation_mode();

/**
 * Handles an incoming byte from the mouse.
 *
 * @param b
 * 		value received from the mouse
 */
void handle_mouse_data(uint8_t b);

/**
 * Handles an incoming byte from the keyboard.
 *
 * @param b
 * 		value received from the keyboard
 */
void handle_keyboard_data(uint8_t b);

#endif
