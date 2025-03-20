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

// TODO change values
#define G_USER_MAXIMUM_HEAP_BREAK				    0xA0000000
#define G_USER_VIRTUAL_RANGES_START			        0xA0000000
#define G_USER_VIRTUAL_RANGES_END				    0xC0000000

#define G_KERNEL_AREA_START					        0xC0000000


// TODO new constants
#define G_MEM_LOWER_END                             0x0000000000100000
#define G_MEM_LOWER_HALF_END                        0x00007fffffffffff
#define G_MEM_HIGHER_HALF_DIRECT_MAP_OFFSET         0xffff800000000000
#define G_MEM_PHYS_TO_VIRT(phys)                    ((G_MEM_HIGHER_HALF_DIRECT_MAP_OFFSET) + (g_address) phys)

#define G_MEM_KERN_VIRT_RANGES_START			    0x00000009f0000000
#define G_MEM_KERN_VIRT_RANGES_END			        0x00000009ffc00000
#define G_MEM_HEAP_START                            0x0000000a00000000
#define G_MEM_HEAP_INITIAL_SIZE                     0x0000000000100000
#define G_KERNEL_HEAP_EXPAND_STEP			    	0x0000000000100000


#endif
