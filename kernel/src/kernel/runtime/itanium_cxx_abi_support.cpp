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

#include "kernel/memory/heap.hpp"
#include "kernel/panic.hpp"

// The functions in this file are implemented as defined in the Itanium C++ ABI
// standard. These functions are required by GCC for special cases, see the
// individual documentation for details.

namespace std
{
	[[noreturn]] void __throw_bad_function_call()
	{
		panic("bad function call");
	}
}

void* operator new(size_t size, void* ptr) noexcept
{
	return ptr;
}

void* operator new[](size_t size, void* ptr) noexcept
{
	return ptr;
}

void* operator new(size_t size)
{
	return heapAllocate(size);
}

void* operator new[](size_t size)
{
	return heapAllocate(size);
}

void operator delete(void* m)
{
	heapFree(m);
}

void operator delete[](void* m)
{
	heapFree(m);
}

/**
 * This method is called by the GCC if a pure virtual method is called.
 */
extern "C" void __cxa_pure_virtual()
{
	// We can't fix this, so we just do nothing
}

