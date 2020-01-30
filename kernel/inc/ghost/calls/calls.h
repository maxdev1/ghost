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

#ifndef GHOST_API_CALLS_CALLS
#define GHOST_API_CALLS_CALLS

#include "ghost/common.h"
#include "ghost/calls/calls_memory.hpp"
#include "ghost/calls/calls_misc.hpp"
#include "ghost/calls/calls_messaging.hpp"
#include "ghost/calls/calls_spawning.hpp"
#include "ghost/calls/calls_tasking.hpp"
#include "ghost/calls/calls_vm86.hpp"
#include "ghost/calls/calls_filesystem.hpp"

__BEGIN_C

#define G_SYSCALL_EXIT							1
#define G_SYSCALL_YIELD							2
#define G_SYSCALL_GET_PROCESS_ID				3
#define G_SYSCALL_GET_TASK_ID					4
#define G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID	5
#define G_SYSCALL_FORK							6
#define G_SYSCALL_JOIN							7
#define G_SYSCALL_SLEEP							8
#define G_SYSCALL_ATOMIC_LOCK					9
#define G_SYSCALL_LOG							10
#define G_SYSCALL_SET_VIDEO_LOG					11
#define G_SYSCALL_TEST							12
#define G_SYSCALL_RELEASE_CLI_ARGUMENTS			13
#define G_SYSCALL_GET_WORKING_DIRECTORY			14
#define G_SYSCALL_SET_WORKING_DIRECTORY			15
#define G_SYSCALL_KILL							16
#define G_SYSCALL_REGISTER_IRQ_HANDLER			18
#define G_SYSCALL_RESTORE_INTERRUPTED_STATE		19
#define G_SYSCALL_REGISTER_SIGNAL_HANDLER		20
#define G_SYSCALL_RAISE_SIGNAL					21
#define G_SYSCALL_KERNQUERY						23
#define G_SYSCALL_GET_EXECUTABLE_PATH			24
#define G_SYSCALL_GET_PARENT_PROCESS_ID			25
#define G_SYSCALL_TASK_GET_TLS                  27
#define G_SYSCALL_PROCESS_GET_INFO              28

#define G_SYSCALL_CALL_VM86						50
#define G_SYSCALL_LOWER_MEMORY_ALLOCATE			51
#define G_SYSCALL_LOWER_MEMORY_FREE				52
#define G_SYSCALL_ALLOCATE_MEMORY				53
#define G_SYSCALL_UNMAP							54
#define G_SYSCALL_SHARE_MEMORY					55
#define G_SYSCALL_MAP_MMIO_AREA					56
#define G_SYSCALL_SBRK							57

#define G_SYSCALL_SPAWN							70
#define G_SYSCALL_CREATE_THREAD					71
#define G_SYSCALL_GET_THREAD_ENTRY				72

#define G_SYSCALL_REGISTER_TASK_IDENTIFIER		90
#define G_SYSCALL_GET_TASK_FOR_IDENTIFIER		91
#define G_SYSCALL_MESSAGE_SEND					92
#define G_SYSCALL_MESSAGE_RECEIVE				93

#define G_SYSCALL_GET_MILLISECONDS				100

#define G_SYSCALL_FS_OPEN						120
#define G_SYSCALL_FS_READ						121
#define G_SYSCALL_FS_WRITE						122
#define G_SYSCALL_FS_CLOSE						123
#define G_SYSCALL_FS_STAT						124
#define G_SYSCALL_FS_FSTAT						125
#define G_SYSCALL_FS_CLONEFD					126
#define G_SYSCALL_FS_PIPE						127
#define G_SYSCALL_FS_LENGTH						128
#define G_SYSCALL_FS_SEEK						129
#define G_SYSCALL_FS_TELL						130
#define G_SYSCALL_FS_REGISTER_AS_DELEGATE		131
#define G_SYSCALL_FS_SET_TRANSACTION_STATUS		132
#define G_SYSCALL_FS_CREATE_NODE				133
#define G_SYSCALL_FS_OPEN_DIRECTORY				134
#define G_SYSCALL_FS_READ_DIRECTORY				135
#define G_SYSCALL_FS_CLOSE_DIRECTORY			136

#define G_SYSCALL_MAX							150

__END_C

#endif
