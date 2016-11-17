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
#include "ghost/calls/calls_ramdisk.hpp"
#include "ghost/calls/calls_spawning.hpp"
#include "ghost/calls/calls_tasking.hpp"
#include "ghost/calls/calls_vm86.hpp"
#include "ghost/calls/calls_filesystem.hpp"

__BEGIN_C

#define G_SYSCALL_EXIT							0x101
#define G_SYSCALL_YIELD							0x102
#define G_SYSCALL_GET_PROCESS_ID				0x103
#define G_SYSCALL_GET_TASK_ID					0x104
#define G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID	0x105
#define G_SYSCALL_FORK							0x106
#define G_SYSCALL_JOIN							0x107
#define G_SYSCALL_SLEEP							0x108
#define G_SYSCALL_ATOMIC_LOCK					0x109
#define G_SYSCALL_WAIT_FOR_IRQ					0x10A
#define G_SYSCALL_LOG							0x10B
#define G_SYSCALL_SET_VIDEO_LOG					0x10C
#define G_SYSCALL_TEST							0x10D
#define G_SYSCALL_STORE_CLI_ARGUMENTS			0x10E
#define G_SYSCALL_RELEASE_CLI_ARGUMENTS			0x10F
#define G_SYSCALL_GET_WORKING_DIRECTORY			0x110
#define G_SYSCALL_SET_WORKING_DIRECTORY			0x111
#define G_SYSCALL_KILL							0x112
#define G_SYSCALL_ATOMIC_BLOCK					0x113
#define G_SYSCALL_REGISTER_IRQ_HANDLER			0x114
#define G_SYSCALL_RESTORE_INTERRUPTED_STATE		0x115
#define G_SYSCALL_REGISTER_SIGNAL_HANDLER		0x116
#define G_SYSCALL_RAISE_SIGNAL					0x117
#define G_SYSCALL_ATOMIC_TRY_LOCK				0x118
#define G_SYSCALL_KERNQUERY						0x119
#define G_SYSCALL_GET_EXECUTABLE_PATH			0x11A
#define G_SYSCALL_GET_PARENT_PROCESS_ID			0x11B
#define G_SYSCALL_EXIT_THREAD					0x11C

#define G_SYSCALL_CALL_VM86						0x201
#define G_SYSCALL_LOWER_MEMORY_ALLOCATE			0x202
#define G_SYSCALL_LOWER_MEMORY_FREE				0x203
#define G_SYSCALL_ALLOCATE_MEMORY				0x204
#define G_SYSCALL_UNMAP							0x205
#define G_SYSCALL_SHARE_MEMORY					0x206
#define G_SYSCALL_MAP_MMIO_AREA					0x207
#define G_SYSCALL_SBRK							0x208

#define G_SYSCALL_RAMDISK_SPAWN					0x301
#define G_SYSCALL_CREATE_THREAD					0x302
#define G_SYSCALL_GET_THREAD_ENTRY				0x303
#define G_SYSCALL_CREATE_EMPTY_PROCESS			0x304
#define G_SYSCALL_CREATE_PAGES_IN_SPACE			0x305
#define G_SYSCALL_ATTACH_CREATED_PROCESS		0x306
#define G_SYSCALL_CANCEL_PROCESS_CREATION		0x307
#define G_SYSCALL_GET_CREATED_PROCESS_ID		0x308
#define G_SYSCALL_WRITE_TLS_MASTER_FOR_PROCESS	0x309
#define G_SYSCALL_CONFIGURE_PROCESS				0x30A

#define G_SYSCALL_REGISTER_TASK_IDENTIFIER		0x401
#define G_SYSCALL_GET_TASK_FOR_IDENTIFIER		0x402
#define G_SYSCALL_MESSAGE_SEND					0x403
#define G_SYSCALL_MESSAGE_RECEIVE				0x404
#define G_SYSCALL_MESSAGE_RECEIVE_TRANSACTION	0x405

#define G_SYSCALL_RAMDISK_FIND					0x501
#define G_SYSCALL_RAMDISK_FIND_CHILD			0x502
#define G_SYSCALL_RAMDISK_INFO					0x503
#define G_SYSCALL_RAMDISK_READ					0x504
#define G_SYSCALL_RAMDISK_CHILD_COUNT			0x505
#define G_SYSCALL_RAMDISK_CHILD_AT				0x506
#define G_SYSCALL_GET_MILLISECONDS				0x507

#define G_SYSCALL_FS_OPEN						0x601
#define G_SYSCALL_FS_READ						0x602
#define G_SYSCALL_FS_WRITE						0x603
#define G_SYSCALL_FS_CLOSE						0x604
#define G_SYSCALL_FS_STAT						0x605
#define G_SYSCALL_FS_FSTAT						0x606
#define G_SYSCALL_FS_CLONEFD					0x607
#define G_SYSCALL_FS_PIPE						0x608
#define G_SYSCALL_FS_LENGTH						0x609
#define G_SYSCALL_FS_SEEK						0x60A
#define G_SYSCALL_FS_TELL						0x60B
#define G_SYSCALL_FS_REGISTER_AS_DELEGATE		0x60C
#define G_SYSCALL_FS_SET_TRANSACTION_STATUS		0x60D
#define G_SYSCALL_FS_CREATE_NODE				0x60E
#define G_SYSCALL_FS_OPEN_DIRECTORY				0x60F
#define G_SYSCALL_FS_READ_DIRECTORY				0x610
#define G_SYSCALL_FS_CLOSE_DIRECTORY			0x611

#define G_SYSCALL_MAX							0x1000

__END_C

#endif
