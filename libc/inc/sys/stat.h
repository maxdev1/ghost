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

#ifndef __GHOST_LIBC_SYS_STAT__
#define __GHOST_LIBC_SYS_STAT__

#include "ghost/stdint.h"

typedef uint32_t mode_t;

// POSIX stat flags
// TODO kernel must understand these
#define S_IXOTH 01
#define S_IWOTH 02
#define S_IROTH 03
#define S_IRWXO 07
#define S_IXGRP 010
#define S_IWGRP 020
#define S_IRGRP 040
#define S_IRWXG 070
#define S_IXUSR 0100
#define S_IWUSR 0200
#define S_IRUSR 0400
#define S_IRWXU 0700

#endif
