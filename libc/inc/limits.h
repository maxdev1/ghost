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

#ifndef __GHOST_LIBC_LIMITS__
#define __GHOST_LIBC_LIMITS__

// This header originates from the musl C library http://www.musl-libc.org/

/* Most limits are system-specific */
#ifdef __x86_x64__
#error "architecture not supported"

#elif __i386__
#define LONG_BIT		32
#define LONG_MAX		0x7fffffffL
#define LLONG_MAX		0x7fffffffffffffffLL

/* Support signed or unsigned plain-char */
#if '\0'-1 > 0
#define CHAR_MIN		0
#define CHAR_MAX		255
#else
#define CHAR_MIN		(-128)
#define CHAR_MAX		127
#endif

/* Some universal constants... */
#define CHAR_BIT		8
#define SCHAR_MIN		(-128)
#define SCHAR_MAX		127
#define UCHAR_MAX		255
#define SHRT_MIN		(-1 - 0x7fff)
#define SHRT_MAX		0x7fff
#define USHRT_MAX		0xffff
#define INT_MIN			(-1 - 0x7fffffff)
#define INT_MAX			0x7fffffff
#define UINT_MAX		0xffffffffU
#define LONG_MIN		(-LONG_MAX - 1)
#define ULONG_MAX		(2UL * LONG_MAX + 1)
#define LLONG_MIN		(-LLONG_MAX - 1)
#define ULLONG_MAX		(2ULL * LLONG_MAX + 1)

#define MB_LEN_MAX		4

#define PIPE_BUF		G_PIPE_DEFAULT_CAPACITY
#define FILESIZEBITS	64

#define NAME_MAX		G_FILENAME_MAX
#define SYMLINK_MAX		G_FILENAME_MAX
#define PATH_MAX		G_PATH_MAX

#define NZERO			20
#define NGROUPS_MAX		32
#define ARG_MAX			131072
#define IOV_MAX			1024
#define SYMLOOP_MAX		40
#define WORD_BIT		32
#define SSIZE_MAX		LONG_MAX
#define TZNAME_MAX		6
#define TTY_NAME_MAX	32
#define HOST_NAME_MAX	255

/* Arbitrary numbers... */
#define BC_BASE_MAX				99
#define BC_DIM_MAX				2048
#define BC_SCALE_MAX			99
#define BC_STRING_MAX			1000
#define CHARCLASS_NAME_MAX		14
#define COLL_WEIGHTS_MAX		2
#define EXPR_NEST_MAX			32
#define LINE_MAX				4096
#define RE_DUP_MAX				255

#define NL_ARGMAX				9
#define NL_LANGMAX				32
#define NL_MSGMAX				32767
#define NL_SETMAX				255
#define NL_TEXTMAX				2048

#define NL_NMAX					16

#else
#error "architecture not supported"
#endif


#endif
