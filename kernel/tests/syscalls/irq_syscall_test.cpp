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

#include "support/fixture.hpp"
#define GHOST_API_STDINT
#include <ghost/system/irq.h>
#include <ghost/tasks/types.h>

namespace
{
#include "production/irq_syscall_types.inc"

struct g_task
{
	int securityLevel;
};

g_task driver{G_SECURITY_LEVEL_DRIVER}, application{G_SECURITY_LEVEL_APPLICATION};
constexpr g_irq_handle binding = 0x1234567811ULL;
g_irq_source boundSource;
uint32_t boundLine, waitedMs, releases, installs;
bool bound, masked, writable;

bool memoryCopyFromUser(g_task*, void* result, g_address user, size_t bytes)
{
	if(!user)
		return false;
	memcpy(result, reinterpret_cast<void*>(user), bytes);
	return true;
}

bool memoryCopyToUser(g_task*, g_address user, const void* result, size_t bytes)
{
	if(!writable)
		return false;
	memcpy(reinterpret_cast<void*>(user), result, bytes);
	return true;
}

g_irq_status irqInstall(g_task*, g_irq_source source, uint32_t line, g_irq_handle* out)
{
	++installs;
	if(bound)
		return G_IRQ_BUSY;
	bound = true;
	masked = true;
	boundSource = source;
	boundLine = line;
	*out = binding;
	return G_IRQ_SUCCESSFUL;
}

g_irq_status irqWait(g_task*, g_irq_handle handle, uint32_t timeoutMs)
{
	waitedMs = timeoutMs;
	if(!bound || handle != binding)
		return G_IRQ_RELEASED;
	return timeoutMs ? G_IRQ_TIMED_OUT : G_IRQ_DELIVERED;
}

g_irq_status irqComplete(g_task*, g_irq_handle handle)
{
	if(!bound || handle != binding)
		return G_IRQ_RELEASED;
	masked = false;
	return G_IRQ_SUCCESSFUL;
}

g_irq_status irqMask(g_task*, g_irq_handle handle)
{
	if(!bound || handle != binding)
		return G_IRQ_RELEASED;
	masked = true;
	return G_IRQ_SUCCESSFUL;
}

g_irq_status irqRelease(g_task*, g_irq_handle handle)
{
	if(!bound || handle != binding)
		return G_IRQ_RELEASED;
	++releases;
	bound = false;
	return G_IRQ_SUCCESSFUL;
}

#include "production/irq_syscalls.inc"

#include "support/memory_user.hpp"
#include "support/syscall_dispatch.hpp"

class IrqSyscall : public testing::Test
{
protected:
	void SetUp() override
	{
		bound = false;
		writable = true;
		installs = releases = boundLine = waitedMs = 0;
	}
};

TEST_F(IrqSyscall, NativeRequestsPreserveFullHandleAndTimeout)
{
	g_syscall_irq request{};
	request.operation = G_IRQ_BIND;
	request.source = G_IRQ_SOURCE_ISA;
	request.line = 12;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	ASSERT_EQ(request.status, G_IRQ_SUCCESSFUL);
	ASSERT_EQ(uint64_t(request.handle), binding);
	EXPECT_EQ(boundSource, G_IRQ_SOURCE_ISA);
	EXPECT_EQ(boundLine, 12u);

	request.operation = G_IRQ_COMPLETE;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	ASSERT_EQ(request.status, G_IRQ_SUCCESSFUL);
	EXPECT_FALSE(masked);
	request.operation = G_IRQ_MASK;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	ASSERT_EQ(request.status, G_IRQ_SUCCESSFUL);
	EXPECT_TRUE(masked);

	request.operation = G_IRQ_WAIT;
	request.timeoutMs = UINT32_MAX;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_EQ(request.status, G_IRQ_TIMED_OUT);
	EXPECT_EQ(waitedMs, UINT32_MAX);

	request.operation = G_IRQ_RELEASE;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	ASSERT_EQ(request.status, G_IRQ_SUCCESSFUL);
	request.operation = G_IRQ_WAIT;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_EQ(request.status, G_IRQ_RELEASED);
}

TEST_F(IrqSyscall, RejectsUnknownOperationsAndUnprivilegedBindsWithoutInstalling)
{
	g_syscall_irq request{};
	request.operation = UINT8_MAX;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_EQ(request.status, G_IRQ_INVALID_ARGUMENT);

	request.operation = G_IRQ_BIND;
	request.source = G_IRQ_SOURCE_ISA;
	request.line = 1;
	syscallTestInvoke(&application, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_EQ(request.status, G_IRQ_NOT_PERMITTED);
	EXPECT_EQ(installs, 0u);
}

TEST_F(IrqSyscall, RejectsUndeliverableRepliesBeforeInstallingABinding)
{
	writable = false;
	g_syscall_irq request{};
	request.operation = G_IRQ_BIND;
	request.source = G_IRQ_SOURCE_ISA;
	request.line = 1;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_FALSE(bound);
	EXPECT_EQ(installs, 0u);
	EXPECT_EQ(releases, 0u);

	bound = true;
	syscallTestInvoke(&driver, (g_syscall_handler) syscallIrq, &request, sizeof(g_syscall_irq));
	EXPECT_TRUE(bound);
	EXPECT_EQ(releases, 0u);
}
}
