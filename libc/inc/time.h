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

#ifndef __GHOST_LIBC_TIME__
#define __GHOST_LIBC_TIME__

#include "ghost/common.h"
#include <stdio.h>

__BEGIN_C

/**
 *
 */
typedef long time_t;
typedef long suseconds_t;

typedef void* timer_t;
typedef int clockid_t;
typedef long clock_t;

#define CLOCKS_PER_SEC 1000000L

#define TIME_UTC 1

/**
 *
 */
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long __tm_gmtoff;
	const char *__tm_zone;
};

/**
 *
 */
clock_t clock();

/**
 *
 */
time_t time(time_t *);

/**
 *
 */
double difftime(time_t, time_t);

/**
 *
 */
time_t mktime(struct tm *);

/**
 *
 */
size_t strftime(char *, size_t, const char*, const struct tm *);

/**
 *
 */
struct tm* gmtime(const time_t *);

/**
 *
 */
struct tm* localtime(const time_t *);

/**
 *
 */
char* asctime(const struct tm *);

/**
 *
 */
char* ctime(const time_t *);

__END_C

#endif
