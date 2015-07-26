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

#include <ghost.h>
#include <ghostuser/utils/logger.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

// Fix for CYGWIN
#ifndef vsnprintf
extern "C" int vsnprintf(char *__restrict, size_t, const char *__restrict, va_list v);
#endif

/**
 *
 */
void g_logger::log(std::string message, ...) {
	va_list l;
	va_start(l, message);

	log(message.c_str(), l);

	va_end(l);
}

/**
 *
 */
void g_logger::log(const char* message, ...) {
	va_list l;
	va_start(l, message);

	log(message, l);

	va_end(l);
}

/**
 *
 */
void g_logger::log(const char* message, va_list l) {
	uint32_t msglen = strlen(message);
	uint32_t buflen = msglen * 2;
	char* buf = new char[buflen];
	vsnprintf(buf, buflen, message, l);
	g_log(buf);
	delete buf;
}

