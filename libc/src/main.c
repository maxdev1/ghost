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

#include "ghost.h"
#include "malloc.h"
#include "locale.h"
#include "errno.h"
#include "stdlib.h"
#include "signal.h"

#include "main_internal.h"
#include "stdio/stdio_internal.h"

/**
 * Global ctor/dtor functions
 */
extern void _init();
extern void _fini();

extern void (*__init_array_start[])();
extern void (*__init_array_end[])();
extern void (*__preinit_array_start[])();
extern void (*__preinit_array_end[])();
extern void (*__fini_array_start[])();
extern void (*__fini_array_end[])();

/**
 * Application entry routine, called by the CRTs.
 */
int __g_main()
{

#warning "Test before syscalls get implemented"
	for(;;)
	{
		g_sleep(100);
	}

	// initialize libc
	__g_init_libc();

	// default return value
	int ret = -1;

	// parse arguments and call application main
	int argc;
	char** args;
	if(parseargs(&argc, &args) == 0)
	{
		ret = main(argc, args);
	} else
	{
		g_log("failed to parse command line arguments");
	}

	// leave
	exit(ret);
}

/**
 * Initializes the C library
 */
void __g_init_libc()
{

	// call global constructors
	for(size_t i = 0; i < __preinit_array_end - __preinit_array_start; i++)
	{
		(*__preinit_array_start[i])();
	}

	_init();

	for(size_t i = 0; i < __init_array_end - __init_array_start; i++)
	{
		(*__init_array_start[i])();
	}

	// set default locale (N1548-7.11.1.1-4)
	setlocale(LC_ALL, "C");

	// set default signal handlers
	signal(SIGINT, SIG_DFL);

	// initialize standard I/O
	__init_stdio();
}

/**
 * Finalize the C library
 */
void __g_fini_libc()
{

	// call global destructors
	for(size_t i = 0; i < __fini_array_end - __fini_array_start; i++)
	{
		(*__fini_array_start[i])();
	}

	_fini();

	// Finalize the standard I/O
	__fini_stdio();
}

