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

#ifndef __GHOST_LIBC_ASSERT__
#define __GHOST_LIBC_ASSERT__

#include "ghost.h"

__BEGIN_C

// use pretty function from g++
#if defined(__cplusplus) && defined(__GNUC__)
#define __g_assert_function		__PRETTY_FUNCTION__

// use __func__ in C99
#elif __STDC_VERSION__ >= 199901L
#define __g_assert_function		__func__

// otherwise unsupported
#else
#define __g_assert_function		((char *) 0)
#endif


// (N1548-7.2-1) redefining
#undef assert

// if the macro NDEBUG is defined, assert must result in nothing (N1548-7.2-1)
#if defined(NDEBUG)
#define assert(ignore)		((void) 0)
#endif

// macro declaration of assert (N1548-7.2-2)
#if !defined(NDEBUG)
#define assert(expr)		((expr) ? (void) 0 : g_assert(__FILE__, __LINE__, __g_assert_function, #expr))
#endif

#if __G_HAS_STDC11
// remove existing declaration
#undef static_assert
// macro declaration of static_assert (N1548-7.2-3)
#define static_assert		_Static_assert
#endif


// assert function
void g_assert(const char* file, int line, const char* function, const char* expr);


__END_C

#endif
