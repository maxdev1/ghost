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

#ifndef __GHOST_LIBC_SETJMP__
#define __GHOST_LIBC_SETJMP__

#include "ghost/common.h"

__BEGIN_C

// declare for each architecture
#if defined(__x86_64__)
#define __JMP_BUF_LENGTH	8
#define __JMP_BUF_TYPE		unsigned long long

#elif defined(__i386__)
#define __JMP_BUF_LENGTH	6
#define __JMP_BUF_TYPE		unsigned long

#else
#error "architecture not supported"

#endif

// check definitions
#if !defined(__JMP_BUF_LENGTH)
#error "no architecture-specific 'jmp_buf' length defined"
#endif
#if !defined(__JMP_BUF_TYPE)
#error "no architecture-specific 'jmp_buf' type defined"
#endif

// (N1548-7.13-2)
typedef __JMP_BUF_TYPE jmp_buf[__JMP_BUF_LENGTH];

/**
 * Saves the calling environment in the given <env> buffer. (N1548-7.13.1.1)
 *
 * @param env
 * 		buffer to save the environment in
 * @return
 * 		zero if from a direct call; non-zero if from a call to <longjmp>
 */
int setjmp(jmp_buf env);


/**
 * Restores the calling environment from the given <env> buffer. (N1548-7.13.2.1)
 *
 * @param env
 * 		buffer to restore the environment from
 */
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

__END_C

#endif
