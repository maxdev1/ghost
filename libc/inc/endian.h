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

#ifndef __GHOST_LIBC_ENDIAN__
#define __GHOST_LIBC_ENDIAN__

#include "ghost/common.h"
#include <stdint.h>

__BEGIN_C

// when using GCC, define the order
#if defined(__GNUC__) && defined(__BYTE_ORDER__)
#define __LITTLE_ENDIAN		__ORDER_LITTLE_ENDIAN__
#define __BIG_ENDIAN		__ORDER_BIG_ENDIAN__
#define __PDP_ENDIAN		__ORDER_PDP_ENDIAN__
#else
#define __LITTLE_ENDIAN		0x01020304
#define __BIG_ENDIAN		0x04030201
#define __PDP_ENDIAN		0x03040102
#endif

// define current byte order
#if defined(__GNUC__) && defined(__BYTE_ORDER__)
#define __BYTE_ORDER		__BYTE_ORDER__

#elif __x86_64__
#define __BYTE_ORDER		__LITTLE_ENDIAN

#elif __i386__
#define __BYTE_ORDER		__LITTLE_ENDIAN

#else
#error "unable to declare endianness for this platform"
#endif

// define other names
#define BIG_ENDIAN			__BIG_ENDIAN
#define LITTLE_ENDIAN		__LITTLE_ENDIAN
#define PDP_ENDIAN			__PDP_ENDIAN
#define BYTE_ORDER			__BYTE_ORDER

/**
 *
 */
static __inline uint16_t __bswap16(uint16_t __x) {
	return __x << 8 | __x >> 8;
}

/**
 *
 */
static __inline uint32_t __bswap32(uint32_t __x) {
	return __x >> 24 | (__x >> 8 & 0xff00) | (__x << 8 & 0xff0000) | __x << 24;
}

/**
 *
 */
static __inline uint64_t __bswap64(uint64_t __x) {
	return __bswap32(__x) + 0ULL << 32 | __bswap32(__x >> 32);
}

/**
 *
 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe16(x) __bswap16(x)
#define be16toh(x) __bswap16(x)
#define betoh16(x) __bswap16(x)
#define htobe32(x) __bswap32(x)
#define be32toh(x) __bswap32(x)
#define betoh32(x) __bswap32(x)
#define htobe64(x) __bswap64(x)
#define be64toh(x) __bswap64(x)
#define betoh64(x) __bswap64(x)
#define htole16(x) (uint16_t)(x)
#define le16toh(x) (uint16_t)(x)
#define letoh16(x) (uint16_t)(x)
#define htole32(x) (uint32_t)(x)
#define le32toh(x) (uint32_t)(x)
#define letoh32(x) (uint32_t)(x)
#define htole64(x) (uint64_t)(x)
#define le64toh(x) (uint64_t)(x)
#define letoh64(x) (uint64_t)(x)
#else
#define htobe16(x) (uint16_t)(x)
#define be16toh(x) (uint16_t)(x)
#define betoh16(x) (uint16_t)(x)
#define htobe32(x) (uint32_t)(x)
#define be32toh(x) (uint32_t)(x)
#define betoh32(x) (uint32_t)(x)
#define htobe64(x) (uint64_t)(x)
#define be64toh(x) (uint64_t)(x)
#define betoh64(x) (uint64_t)(x)
#define htole16(x) __bswap16(x)
#define le16toh(x) __bswap16(x)
#define letoh16(x) __bswap16(x)
#define htole32(x) __bswap32(x)
#define le32toh(x) __bswap32(x)
#define letoh32(x) __bswap32(x)
#define htole64(x) __bswap64(x)
#define le64toh(x) __bswap64(x)
#define letoh64(x) __bswap64(x)
#endif

__END_C

#endif
