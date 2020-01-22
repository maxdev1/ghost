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

void __g_init_libc_call_init();
void __g_fini_libc_call_fini();

/**
 * Application entry routine, called by the CRTs.
 */
int __g_main()
{
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
	// call init functions
	__g_init_libc_call_init();

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
	// call fini functions
	__g_fini_libc_call_fini();

	// Finalize the standard I/O
	__fini_stdio();
}

/**
 * Asks the kernel for the process info and calls all necessary init functions
 * of all objects.
 */
void __g_init_libc_call_init()
{
	g_process_info* processInfo = g_process_get_info();
	for(int i = 0; i < processInfo->objectInfosSize; i++)
	{
		g_object_info* objectInfo = &processInfo->objectInfos[i];
		if(objectInfo->preinitArray)
		{
			for(uint32_t i = 0; i < objectInfo->preinitArraySize; i++)
			{
				klog("%s.preinit[%i]:%x()", objectInfo->name, i, objectInfo->preinitArray[i]);
				objectInfo->preinitArray[i]();
			}
		}

		if(objectInfo->init)
		{
			klog("%s.init:%x()", objectInfo->name, objectInfo->init);
			objectInfo->init();
		}

		if(objectInfo->initArray)
		{
			for(uint32_t i = 0; i < objectInfo->initArraySize; i++)
			{
				klog("%s.init[%i]:%x()", objectInfo->name, i, objectInfo->initArray[i]);
				objectInfo->initArray[i]();
			}
		}
	}
}

/**
 * Asks the kernel for process info and calls all necessary fini functions
 * of all objects.
 */
void __g_fini_libc_call_fini()
{
	g_process_info* processInfo = g_process_get_info();
	for(int i = 0; i < processInfo->objectInfosSize; i++)
	{
		g_object_info* objectInfo = &processInfo->objectInfos[i];
		if(objectInfo->finiArray)
		{
			for(uint32_t i = 0; i < objectInfo->finiArraySize; i++)
			{
				klog("%s.fini[%i]:%x()", objectInfo->name, i, objectInfo->finiArray[i]);
				objectInfo->finiArray[i]();
			}
		}
	}
}
