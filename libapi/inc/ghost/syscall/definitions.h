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

#ifndef GHOST_API_SYSCALL_DEFINITIONS
#define GHOST_API_SYSCALL_DEFINITIONS

#include "../common.h"

__BEGIN_C

// Tasking
#define G_SYSCALL_EXIT							1
#define G_SYSCALL_YIELD							2
#define G_SYSCALL_GET_PROCESS_ID				3
#define G_SYSCALL_GET_TASK_ID					4
#define G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID	5
#define G_SYSCALL_FORK							6
#define G_SYSCALL_JOIN							7
#define G_SYSCALL_SLEEP							8
#define G_SYSCALL_RELEASE_CLI_ARGUMENTS			9
#define G_SYSCALL_GET_WORKING_DIRECTORY			10
#define G_SYSCALL_SET_WORKING_DIRECTORY			11
#define G_SYSCALL_KILL							12
#define G_SYSCALL_GET_EXECUTABLE_PATH			13
#define G_SYSCALL_GET_PARENT_PROCESS_ID			14
#define G_SYSCALL_TASK_GET_TLS                  15
#define G_SYSCALL_PROCESS_GET_INFO              16
#define G_SYSCALL_SPAWN							17
#define G_SYSCALL_CREATE_TASK					18
#define G_SYSCALL_GET_TASK_ENTRY				19
#define G_SYSCALL_EXIT_TASK   				    20
#define G_SYSCALL_REGISTER_TASK_IDENTIFIER		21
#define G_SYSCALL_GET_TASK_FOR_IDENTIFIER		22
#define G_SYSCALL_GET_MILLISECONDS				23

// Memory
#define G_SYSCALL_LOWER_MEMORY_ALLOCATE			40
#define G_SYSCALL_LOWER_MEMORY_FREE				41
#define G_SYSCALL_ALLOCATE_MEMORY				42
#define G_SYSCALL_UNMAP							43
#define G_SYSCALL_SHARE_MEMORY					44
#define G_SYSCALL_MAP_MMIO_AREA					45
#define G_SYSCALL_SBRK							46

// Mutex
#define G_SYSCALL_USER_MUTEX_INITIALIZE 		60
#define G_SYSCALL_USER_MUTEX_ACQUIRE			61
#define G_SYSCALL_USER_MUTEX_RELEASE			62
#define G_SYSCALL_USER_MUTEX_DESTROY			63

// Messages
#define G_SYSCALL_MESSAGE_SEND                  70
#define G_SYSCALL_MESSAGE_RECEIVE				71
#define G_SYSCALL_MESSAGE_NEXT_TXID				72

// Filesystem
#define G_SYSCALL_FS_OPEN						80
#define G_SYSCALL_FS_READ						81
#define G_SYSCALL_FS_WRITE						82
#define G_SYSCALL_FS_CLOSE						83
#define G_SYSCALL_FS_STAT						84
#define G_SYSCALL_FS_FSTAT						85
#define G_SYSCALL_FS_CLONEFD					86
#define G_SYSCALL_FS_PIPE						87
#define G_SYSCALL_FS_LENGTH						88
#define G_SYSCALL_FS_SEEK						89
#define G_SYSCALL_FS_TELL						90
#define G_SYSCALL_FS_REGISTER_AS_DELEGATE		91
#define G_SYSCALL_FS_SET_TRANSACTION_STATUS		92
#define G_SYSCALL_FS_CREATE_NODE				93
#define G_SYSCALL_FS_OPEN_DIRECTORY				94
#define G_SYSCALL_FS_READ_DIRECTORY				95
#define G_SYSCALL_FS_CLOSE_DIRECTORY			96
#define G_SYSCALL_OPEN_IRQ_DEVICE   			97

// System
#define G_SYSCALL_CALL_VM86						120
#define G_SYSCALL_LOG							121
#define G_SYSCALL_SET_VIDEO_LOG					122
#define G_SYSCALL_TEST							123
#define G_SYSCALL_IRQ_CREATE_REDIRECT           124

// Kernquery
#define G_SYSCALL_KERNQUERY						129

#define G_SYSCALL_MAX							130

__END_C

#endif
