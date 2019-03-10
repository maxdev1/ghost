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

#ifndef __GHOST_SYS_STDINT__
#define __GHOST_SYS_STDINT__

#include <stddef.h>
#include "ghost/common.h"

__BEGIN_C

// exact-width integer types (N1546-7.20.1.1)
typedef __UINT8_TYPE__			uint8_t;
typedef __INT8_TYPE__			int8_t;
typedef __UINT16_TYPE__			uint16_t;
typedef __INT16_TYPE__			int16_t;
typedef __UINT32_TYPE__			uint32_t;
typedef __INT32_TYPE__			int32_t;
typedef __UINT64_TYPE__			uint64_t;
typedef __INT64_TYPE__			int64_t;

// minimum-width integer types (N1548-7.20.1.2)
typedef __INT_LEAST8_TYPE__		int_least8_t;
typedef __UINT_LEAST8_TYPE__	uint_least8_t;
typedef __INT_LEAST16_TYPE__	int_least16_t;
typedef __UINT_LEAST16_TYPE__	uint_least16_t;
typedef __INT_LEAST32_TYPE__	int_least32_t;
typedef __UINT_LEAST32_TYPE__	uint_least32_t;
typedef __INT_LEAST64_TYPE__	int_least64_t;
typedef __UINT_LEAST64_TYPE__	uint_least64_t;

// fastest minimum-width integer types (N1548-7.20.1.3)
typedef __INT_FAST8_TYPE__		int_fast8_t;
typedef __UINT_FAST8_TYPE__		uint_fast8_t;
typedef __INT_FAST16_TYPE__		int_fast16_t;
typedef __UINT_FAST16_TYPE__	uint_fast16_t;
typedef __INT_FAST32_TYPE__		int_fast32_t;
typedef __UINT_FAST32_TYPE__	uint_fast32_t;
typedef __INT_FAST64_TYPE__		int_fast64_t;
typedef __UINT_FAST64_TYPE__	uint_fast64_t;

// integer type capable of holding the value of a pointer (N1548-7.20.1.4)
typedef __INTPTR_TYPE__			intptr_t;
typedef __UINTPTR_TYPE__		uintptr_t;

// greatest-width integer types (N1548-7.20.1.5)
typedef __INTMAX_TYPE__			intmax_t;
typedef __UINTMAX_TYPE__		uintmax_t;

// limits of exact-width integer types (N1548-7.20.2.1)
#define INT8_MAX				(__INT8_MAX__)
#define INT8_MIN				(-__INT8_MAX__ - 1)
#define INT16_MAX				(__INT16_MAX__)
#define INT16_MIN				(-__INT16_MAX__ - 1)
#define INT32_MAX				(__INT32_MAX__)
#define INT32_MIN				(-__INT32_MAX__ - 1)
#define INT64_MAX				(__INT64_MAX__)
#define INT64_MIN				(-__INT64_MAX__ - 1)
#define UINT8_MAX				(__UINT8_MAX__)
#define UINT16_MAX				(__UINT16_MAX__)
#define UINT32_MAX				(__UINT32_MAX__)
#define UINT64_MAX				(__UINT64_MAX__)

// limits of minimum-width integer types (N1548-7.20.2.2)
#define INT_LEAST8_MAX			(__INT_LEAST8_MAX__)
#define INT_LEAST8_MIN			(-__INT_LEAST8_MAX__ - 1)
#define INT_LEAST16_MAX			(__INT_LEAST16_MAX__)
#define INT_LEAST16_MIN			(-__INT_LEAST16_MAX__ - 1)
#define INT_LEAST32_MAX			(__INT_LEAST32_MAX__)
#define INT_LEAST32_MIN			(-__INT_LEAST32_MAX__ - 1)
#define INT_LEAST64_MAX			(__INT_LEAST64_MAX__)
#define INT_LEAST64_MIN			(-__INT_LEAST64_MAX__ - 1)
#define UINT_LEAST8_MAX			(__UINT_LEAST8_MAX__)
#define UINT_LEAST16_MAX		(__UINT_LEAST16_MAX__)
#define UINT_LEAST32_MAX		(__UINT_LEAST32_MAX__)
#define UINT_LEAST64_MAX		(__UINT_LEAST64_MAX__)

// limits of fastest minimum-width integer types (N1548-7.20.2.3)
#define INT_FAST8_MAX			(__INT_FAST8_MAX__)
#define INT_FAST8_MIN			(-__INT_FAST8_MAX__ - 1)
#define INT_FAST16_MAX			(__INT_FAST16_MAX__)
#define INT_FAST16_MIN			(-__INT_FAST16_MAX__ - 1)
#define INT_FAST32_MAX			(__INT_FAST32_MAX__)
#define INT_FAST32_MIN			(-__INT_FAST32_MAX__ - 1)
#define INT_FAST64_MAX			(__INT_FAST64_MAX__)
#define INT_FAST64_MIN			(-__INT_FAST64_MAX__ - 1)
#define UINT_FAST8_MAX			(__UINT_FAST8_MAX__)
#define UINT_FAST16_MAX			(__UINT_FAST16_MAX__)
#define UINT_FAST32_MAX			(__UINT_FAST32_MAX__)
#define UINT_FAST64_MAX			(__UINT_FAST64_MAX__)

// limits of integer types capable of holding object pointers (N1548-7.20.2.4)
#define INTPTR_MAX				(__INTPTR_MAX__)
#define INTPTR_MIN				(-__INTPTR_MAX__ - 1)
#define UINTPTR_MAX				(__UINTPTR_MAX__)

// limits of greatest-width integer types (N1548-7.20.2.5)
#define INTMAX_MAX				(__INTMAX_MAX__)
#define INTMAX_MIN				(-__INTMAX_MAX__ - 1)
#define UINTMAX_MAX				(__UINTMAX_MAX__)

// limits of other integer types (N1548-7.20.3)
#define PTRDIFF_MAX				(__PTRDIFF_MAX__)
#define PTRDIFF_MIN				(-__PTRDIFF_MAX__ - 1)

#define SIG_ATOMIC_MAX			(__SIG_ATOMIC_MAX__)
#define SIG_ATOMIC_MIN			(__SIG_ATOMIC_MIN__)

#define SIZE_MAX				(__SIZE_MAX__)

#define WCHAR_MAX				(__WCHAR_MAX__)
#define WCHAR_MIN				(__WCHAR_MIN__)

#define WINT_MAX				(__WINT_MAX__)
#define WINT_MIN				(__WINT_MIN__)

// macros for minimum-width integer constants (N1546-7.20.4.1)
#define INT8_C(c)				(__INT8_C(c))
#define INT16_C(c)				(__INT16_C(c))
#define INT32_C(c)				(__INT32_C(c))
#define INT64_C(c)				(__INT64_C(c))

#define UINT8_C(c)				(__UINT8_C(c))
#define UINT16_C(c)				(__UINT16_C(c))
#define UINT32_C(c)				(__UINT32_C(c))
#define UINT64_C(c)				(__UINT64_C(c))

// macros for greatest-width integer constants (N1546-7.20.4.2)
#define INTMAX_C(c)				(__INTMAX_C(c))
#define UINTMAX_C(c)			(__UINTMAX_C(c))

__END_C

#endif
