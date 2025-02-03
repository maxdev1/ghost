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

#ifndef __MEMORY_CONSTANTS__
#define __MEMORY_CONSTANTS__

#define G_SMP_STARTUP_AREA				            0x00000500
#define G_SMP_STARTUP_AREA_PAGEDIR		            0x00000500	// initial page directory address is put here
#define G_SMP_STARTUP_AREA_AP_ENTRY	                0x00000504	// kernel entry point for AP
#define G_SMP_STARTUP_AREA_AP_COUNTER		      	0x00000508	// counter for stack array indexing
#define G_SMP_STARTUP_AREA_AP_STACK_ARRAY	    	0x0000050C	// array of stacks
#define G_SMP_STARTUP_AREA_CODE_START		    	0x00001000	// must be 000XX000, used for SIPI
#define G_SMP_STARTUP_AREA_END				        0x00007BFF

#define G_LOWER_HEAP_MEMORY_START				    0x00007E00	// area used by the lower memory allocator
#define G_LOWER_HEAP_MEMORY_END				        0x000CFFFF	// for vm86 and other 16bit stuff
#define G_LOWER_MEMORY_END					        0x00100000

#define G_USER_MAXIMUM_HEAP_BREAK				    0xA0000000
#define G_USER_VIRTUAL_RANGES_START			        0xA0000000
#define G_USER_VIRTUAL_RANGES_END				    0xC0000000

#define G_KERNEL_AREA_START					        0xC0000000
#define G_KERNEL_HEAP_INIT_SIZE                     0x01000000
#define G_KERNEL_HEAP_EXPAND_STEP			    	0x00100000
#define G_KERNEL_HEAP_END				    		0xF0000000

#define G_KERNEL_VIRTUAL_RANGES_START			    0xF0000000
#define G_KERNEL_VIRTUAL_RANGES_END			        0xFFC00000

#define G_RECURSIVE_PAGE_DIRECTORY_AREA		        0xFFC00000
#define G_RECURSIVE_PAGE_DIRECTORY_ADDRESS	        0xFFFFF000
#define G_RECURSIVE_PAGE_TABLE(ti)                  (((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti))

#endif
