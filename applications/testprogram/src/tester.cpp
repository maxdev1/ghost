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

#include "tester.hpp"
#include <stdio.h>
#include <ghost.h>

uint8_t overflow() {
	char test[0x2000];
	test[0] = 244;
	return test[0];
}

/**
 *
 */
int main(int argc, char** argv) {
	klog("testing stack overflow...");
	uint8_t x;
	klog("stack is: %x", &x);

	uint8_t y = overflow();
	klog("done %c", y);

	g_tid f = g_fork();
	klog("i am: %i", f);

	return y;
	//run_test(argc, argv);
}
