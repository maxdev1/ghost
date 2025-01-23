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
#define G_SYSCALL_LOWER_MEMORY_ALLOCATE			24
#define G_SYSCALL_LOWER_MEMORY_FREE				25
#define G_SYSCALL_ALLOCATE_MEMORY				26
#define G_SYSCALL_UNMAP							27
#define G_SYSCALL_SHARE_MEMORY					28
#define G_SYSCALL_MAP_MMIO_AREA					29
#define G_SYSCALL_SBRK							30

// Mutex
#define G_SYSCALL_USER_MUTEX_INITIALIZE 		31
#define G_SYSCALL_USER_MUTEX_ACQUIRE			32
#define G_SYSCALL_USER_MUTEX_RELEASE			33
#define G_SYSCALL_USER_MUTEX_DESTROY			34

// Messages
#define G_SYSCALL_MESSAGE_SEND                  35
#define G_SYSCALL_MESSAGE_RECEIVE				36

// Filesystem
#define G_SYSCALL_FS_OPEN						37
#define G_SYSCALL_FS_READ						38
#define G_SYSCALL_FS_WRITE						39
#define G_SYSCALL_FS_CLOSE						40
#define G_SYSCALL_FS_STAT						41
#define G_SYSCALL_FS_FSTAT						42
#define G_SYSCALL_FS_CLONEFD					43
#define G_SYSCALL_FS_PIPE						44
#define G_SYSCALL_FS_LENGTH						45
#define G_SYSCALL_FS_SEEK						46
#define G_SYSCALL_FS_TELL						47
#define G_SYSCALL_FS_REGISTER_AS_DELEGATE		48
#define G_SYSCALL_FS_SET_TRANSACTION_STATUS		49
#define G_SYSCALL_FS_CREATE_NODE				50
#define G_SYSCALL_FS_OPEN_DIRECTORY				51
#define G_SYSCALL_FS_READ_DIRECTORY				52
#define G_SYSCALL_FS_CLOSE_DIRECTORY			53
#define G_SYSCALL_OPEN_IRQ_DEVICE   			54

// System
#define G_SYSCALL_CALL_VM86						55
#define G_SYSCALL_LOG							56
#define G_SYSCALL_SET_VIDEO_LOG					57
#define G_SYSCALL_TEST							58

// Kernquery
#define G_SYSCALL_KERNQUERY						59

#define G_SYSCALL_MAX							60

__END_C

#endif
