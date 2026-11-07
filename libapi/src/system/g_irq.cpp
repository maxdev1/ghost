/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "ghost/syscall.h"
#include "ghost/system.h"
#include "ghost/system/callstructs.h"

static g_irq_status g_irq_call(g_syscall_irq& data, g_irq_operation operation)
{
	data.operation = operation;
	data.status = G_IRQ_NOT_SUPPORTED;
	g_syscall(G_SYSCALL_IRQ, reinterpret_cast<g_address>(&data));
	return data.status;
}

g_irq_status g_irq_bind(g_irq_source source, uint32_t line, g_irq_handle* handle)
{
	if(!handle)
		return G_IRQ_INVALID_ARGUMENT;
	*handle = G_IRQ_HANDLE_NONE;

	g_syscall_irq data{};
	data.source = source;
	data.line = line;
	auto status = g_irq_call(data, G_IRQ_BIND);
	if(status == G_IRQ_SUCCESSFUL)
	{
		if(data.handle == G_IRQ_HANDLE_NONE)
			return G_IRQ_INVALID_ARGUMENT;
		*handle = data.handle;
	}
	return status;
}

static g_irq_status g_irq_handle_call(g_irq_operation operation, g_irq_handle handle, uint32_t timeoutMs = 0)
{
	if(handle == G_IRQ_HANDLE_NONE)
		return G_IRQ_INVALID_ARGUMENT;

	g_syscall_irq data{};
	data.handle = handle;
	data.timeoutMs = timeoutMs;
	return g_irq_call(data, operation);
}

g_irq_status g_irq_wait(g_irq_handle handle, uint32_t timeoutMs)
{
	return g_irq_handle_call(G_IRQ_WAIT, handle, timeoutMs);
}

g_irq_status g_irq_complete(g_irq_handle handle)
{
	return g_irq_handle_call(G_IRQ_COMPLETE, handle);
}

g_irq_status g_irq_mask(g_irq_handle handle)
{
	return g_irq_handle_call(G_IRQ_MASK, handle);
}

g_irq_status g_irq_release(g_irq_handle handle)
{
	return g_irq_handle_call(G_IRQ_RELEASE, handle);
}
