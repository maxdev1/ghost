/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <cstddef>
#include <cstdint>

static __thread uint64_t marker __attribute__((tls_model("local-dynamic"))) = 0x123456789abcdef0;
static __thread uint8_t cache[384] __attribute__((tls_model("local-dynamic")));
static __thread uint32_t initialExec __attribute__((tls_model("initial-exec"))) = 17;

extern "C"
{
__thread uint32_t tlsTestExported = 29;

void* tlsTestLibraryFill(uint8_t value)
{
	marker = 0x123456789abcdef0 ^ value;
	initialExec = value + 17;
	tlsTestExported = value + 29;
	for(size_t i = 0; i < sizeof(cache); ++i)
		cache[i] = value;
	return cache;
}

bool tlsTestLibraryCheck(uint8_t value)
{
	if(marker != (0x123456789abcdef0 ^ value) || initialExec != uint32_t(value + 17) ||
	   tlsTestExported != uint32_t(value + 29))
		return false;
	for(size_t i = 0; i < sizeof(cache); ++i)
		if(cache[i] != value)
			return false;
	return true;
}
}
