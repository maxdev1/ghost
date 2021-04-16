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

#ifndef __LIBPS2_INTERN__
#define __LIBPS2_INTERN__

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
ps2_status_t ps2InitializeMouse();

/**
 * Waits for the PS2 input or output buffer by querying the flags in the status byte.
 */
void ps2WaitForBuffer(ps2_buffer_t mode);

/**
 * IRQ handler that is registered at the kernel to receive interrupt requests.
 *
 * @param value
 * 		value received
 */
void ps2IrqHandler(uint8_t value);

/**
 * Handles an incoming byte from the mouse.
 *
 * @param value
 * 		value received from the mouse
 */
void ps2HandleMouseData(uint8_t value);

/**
 * Writes a byte to the mouse.
 *
 * @param value byte to write
 */
int ps2WriteToMouse(uint8_t value);

#endif
