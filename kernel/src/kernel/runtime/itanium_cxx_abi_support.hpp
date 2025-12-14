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

#ifndef __ITANIUM_CXX_ABI_SUPPORT__
#define __ITANIUM_CXX_ABI_SUPPORT__

// Forward declare before including libstdc++ headers so they see it.
namespace std
{
	[[noreturn]] void __throw_bad_function_call();
}

#include <bits/std_function.h>

// Must be defined because we use -ffreestanding which causes these to not be
// available when trying to use std::function
void* operator new(size_t size, void* ptr) noexcept;
void* operator new[](size_t size, void* ptr) noexcept;

// GCC's libstdc++ expects this helper even without exceptions. Provide our own
// panic-backed implementation (see itanium_cxx_abi_support.cpp).
#endif
