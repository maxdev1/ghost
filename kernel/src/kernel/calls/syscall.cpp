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

#include "kernel/calls/syscall.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"

static g_syscall_registration* syscallRegistrations = 0;

void syscallRegister(int callId, void(*handler)(g_task*, void*), bool threaded) {
    
    if(callId > G_SYSCALL_MAX) {
        kernelPanic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);
    }

    syscallRegistrations[callId].handler = handler;
    syscallRegistrations[callId].threaded = threaded;
}

void syscallRegisterAll() {
    syscallRegistrations = (g_syscall_registration*) heapAllocate(sizeof(g_syscall_registration) * G_SYSCALL_MAX);
    for(int i = 0; i < G_SYSCALL_MAX; i++) {
        syscallRegistrations[i].handler = 0;
    }

    syscallRegister(G_SYSCALL_SLEEP, (void(*)(g_task*, void*)) syscallSleep, false);
}

void syscallHandle(g_task* task) {
    uint32_t callId = task->state->eax;
    void* syscallData = (void*) task->state->ebx;

    if(callId > G_SYSCALL_MAX) {
        logInfo("%! task %i tried to use out-of-range syscall %i", "syscall", task->id, callId);
        return;
    }

    g_syscall_registration* reg = &syscallRegistrations[callId];
    if(reg->handler == 0) {
        logInfo("%! task %i tried to use unknown syscall %i", "syscall", task->id, callId);
        return;
    }

    if(reg->threaded) {
        #warning "TODO implement"
    } else {
        reg->handler(task, syscallData);
    }
}

void syscallSleep(g_task* task, g_syscall_sleep* data) {
    logInfo("Task %i: sleep requested for %i ms", task->id, data->milliseconds);
}
