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

#ifndef __GHOST_LIBC_FENV__
#define __GHOST_LIBC_FENV__

// This header originates from the musl C library http://www.musl-libc.org/

#include "ghost/common.h"

__BEGIN_C

// Definitions for x86_64 platform
#if __x86_64__
#error "not implemented for architecuter x86_64"

// Definitions for x86 platform
#elif __i386__

#define FE_INVALID    1
#define __FE_DENORM   2
#define FE_DIVBYZERO  4
#define FE_OVERFLOW   8
#define FE_UNDERFLOW  16
#define FE_INEXACT    32

#define FE_ALL_EXCEPT 63

#define FE_TONEAREST  0
#define FE_DOWNWARD   0x400
#define FE_UPWARD     0x800
#define FE_TOWARDZERO 0xc00

typedef unsigned short fexcept_t;

typedef struct {
	unsigned short __control_word;
	unsigned short __unused1;
	unsigned short __status_word;
	unsigned short __unused2;
	unsigned short __tags;
	unsigned short __unused3;
	unsigned int __eip;
	unsigned short __cs_selector;
	unsigned int __opcode:11;
	unsigned int __unused4:5;
	unsigned int __data_offset;
	unsigned short __data_selector;
	unsigned short __unused5;
} fenv_t;

#define FE_DFL_ENV      ((const fenv_t *) -1)

// Other architectures are not supported
#else
#error "current architecture not supported"

#endif

/**
 *
 */
int feclearexcept(int);

/**
 *
 */
int fegetexceptflag(fexcept_t *, int);

/**
 *
 */
int feraiseexcept(int);

/**
 *
 */
int fesetexceptflag(const fexcept_t *, int);

/**
 *
 */
int fetestexcept(int);


/**
 *
 */
int fegetround(void);

/**
 *
 */
int fesetround(int);


/**
 *
 */
int fegetenv(fenv_t *);

/**
 *
 */
int feholdexcept(fenv_t *);

/**
 *
 */
int fesetenv(const fenv_t *);

/**
 *
 */
int feupdateenv(const fenv_t *);

__END_C

#endif

