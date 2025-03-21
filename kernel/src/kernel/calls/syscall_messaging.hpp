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

#ifndef __KERNEL_SYSCALL_MESSAGING__
#define __KERNEL_SYSCALL_MESSAGING__

#include "kernel/tasking/tasking.hpp"
#include <ghost/messages/callstructs.h>

void syscallMessageSend(g_task* task, g_syscall_send_message* data);

void syscallMessageReceive(g_task* task, g_syscall_receive_message* data);

void syscallMessageTopicSend(g_task* task, g_syscall_send_topic_message* data);

void syscallMessageTopicReceive(g_task* task, g_syscall_receive_topic_message* data);

void syscallMessageNextTxId(g_task* task, g_syscall_message_next_txid* data);

#endif
