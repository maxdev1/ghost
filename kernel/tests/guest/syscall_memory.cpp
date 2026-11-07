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

#include <ghost.h>
#include <ghost/tasks/callstructs.h>
#include <ghost/filesystem/callstructs.h>
#include <ghost/messages/callstructs.h>
#include <ghost/kernquery/types.h>
#include <ghost/system/callstructs.h>
#include <cstring>
#include <cstdio>

static void syscallTestRequire(bool condition, const char* message)
{
	if(!condition)
	{
		klog("syscall-memory-test: FAIL %s", message);
		g_exit(1);
	}
}

int main()
{
	const uint32_t calls[] = {
		G_SYSCALL_YIELD,
		G_SYSCALL_GET_PROCESS_ID,
		G_SYSCALL_GET_TASK_ID,
		G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID,
		G_SYSCALL_JOIN,
		G_SYSCALL_SLEEP,
		G_SYSCALL_RELEASE_CLI_ARGUMENTS,
		G_SYSCALL_GET_WORKING_DIRECTORY,
		G_SYSCALL_SET_WORKING_DIRECTORY,
		G_SYSCALL_KILL,
		G_SYSCALL_GET_EXECUTABLE_PATH,
		G_SYSCALL_TASK_GET_TLS,
		G_SYSCALL_PROCESS_GET_INFO,
		G_SYSCALL_SPAWN,
		G_SYSCALL_CREATE_TASK,
		G_SYSCALL_GET_TASK_ENTRY,
		G_SYSCALL_TASK_REGISTER_NAME,
		G_SYSCALL_GET_TASK_BY_NAME,
		G_SYSCALL_GET_MILLISECONDS,
		G_SYSCALL_GET_NANOSECONDS,
		G_SYSCALL_TASK_AWAIT_BY_NAME,
		G_SYSCALL_GET_LAUNCH_MODE,
		G_SYSCALL_LOWER_MEMORY_ALLOCATE,
		G_SYSCALL_LOWER_MEMORY_FREE,
		G_SYSCALL_ALLOCATE_MEMORY,
		G_SYSCALL_UNMAP,
		G_SYSCALL_SHARE_MEMORY,
		G_SYSCALL_MAP_MMIO_AREA,
		G_SYSCALL_SBRK,
		G_SYSCALL_USER_MUTEX_INITIALIZE,
		G_SYSCALL_USER_MUTEX_ACQUIRE,
		G_SYSCALL_USER_MUTEX_RELEASE,
		G_SYSCALL_USER_MUTEX_DESTROY,
		G_SYSCALL_MESSAGE_SEND,
		G_SYSCALL_MESSAGE_RECEIVE,
		G_SYSCALL_MESSAGE_NEXT_TXID,
		G_SYSCALL_MESSAGE_TOPIC_SEND,
		G_SYSCALL_MESSAGE_TOPIC_RECEIVE,
		G_SYSCALL_FS_OPEN,
		G_SYSCALL_FS_READ,
		G_SYSCALL_FS_WRITE,
		G_SYSCALL_FS_CLOSE,
		G_SYSCALL_FS_STAT,
		G_SYSCALL_FS_FSTAT,
		G_SYSCALL_FS_CLONEFD,
		G_SYSCALL_FS_PIPE,
		G_SYSCALL_FS_LENGTH,
		G_SYSCALL_FS_SEEK,
		G_SYSCALL_FS_TELL,
		G_SYSCALL_FS_DELEGATE_REGISTER,
		G_SYSCALL_FS_DELEGATE_TRANSACTION_COMPLETE,
		G_SYSCALL_FS_DELEGATE_NODE_CREATE,
		G_SYSCALL_FS_OPEN_DIRECTORY,
		G_SYSCALL_FS_READ_DIRECTORY,
		G_SYSCALL_FS_CREATE_DIRECTORY,
		G_SYSCALL_FS_UNLINK_DIRECTORY,
		G_SYSCALL_FS_REAL_PATH,
		G_SYSCALL_FS_DELEGATE_UNREGISTER,
		G_SYSCALL_FS_UNLINK,
		G_SYSCALL_FS_TRUNCATE,
		G_SYSCALL_FS_SYNC,
		G_SYSCALL_FS_RENAME,
		G_SYSCALL_FS_SYMLINK,
		G_SYSCALL_FS_READLINK,
		G_SYSCALL_CALL_VM86,
		G_SYSCALL_LOG,
		G_SYSCALL_SET_VIDEO_LOG,
		G_SYSCALL_TEST,
		G_SYSCALL_GET_EFI_FRAMEBUFFER,
		G_SYSCALL_OPEN_LOG_PIPE,
		G_SYSCALL_SYSTEM_POWER,
		G_SYSCALL_KERNQUERY,
		G_SYSCALL_ACPI_GET_PLATFORM_INFO,
		G_SYSCALL_ACPI_MAP,
		G_SYSCALL_IRQ
	};
	for(auto call : calls)
		g_syscall(call, 0xffff800000001000ULL);
	syscallTestRequire(g_get_pid() != G_PID_NONE, "invalid outer requests recover");

	g_syscall_get_pid pid{};
	pid.id = 123456;
	asm volatile("int $0x80" :: "a" (uint64_t(0x100000000ULL + G_SYSCALL_GET_PROCESS_ID)), "D" (&pid) : "memory");
	syscallTestRequire(pid.id == 123456, "wide syscall number rejected");

	auto page = (uint8_t*) g_alloc_mem(G_PAGE_SIZE);
	auto hole = g_alloc_mem(G_PAGE_SIZE);
	syscallTestRequire(page && hole == page + G_PAGE_SIZE, "adjacent fixture mappings");
	g_unmap(hole);
	page[G_PAGE_SIZE - 2] = 0xaa;
	const uint32_t boundaryCalls[] = {
		G_SYSCALL_GET_PROCESS_ID, G_SYSCALL_ACPI_GET_PLATFORM_INFO, G_SYSCALL_ACPI_MAP
	};
	for(auto call : boundaryCalls)
	{
		g_syscall(call, (g_address) (page + G_PAGE_SIZE - 2));
		syscallTestRequire(page[G_PAGE_SIZE - 2] == 0xaa, "cross-page request rejected before output");
	}

	g_fd writeFd, readFd;
	syscallTestRequire(g_pipe(&writeFd, &readFd) == G_FS_PIPE_SUCCESSFUL, "pipe creation");
	syscallTestRequire(g_write(writeFd, "abc", 3) == 3, "pipe data queued");
	g_fs_read_status readStatus;
	syscallTestRequire(g_read_s(readFd, page + G_PAGE_SIZE - 2, 3, &readStatus) == -1 &&
	                   readStatus == G_FS_READ_ERROR, "cross-page read rejected");
	char bytes[4]{};
	syscallTestRequire(g_read(readFd, bytes, 3) == 3 && !memcmp(bytes, "abc", 3), "failed read preserves pipe");
	g_close(writeFd);
	g_close(readFd);

	syscallTestRequire(g_send_message(g_get_tid(), (void*) "abc", 3) == G_MESSAGE_SEND_STATUS_SUCCESSFUL, "self message");
	syscallTestRequire(g_receive_message_m(page + G_PAGE_SIZE - 2, sizeof(g_message_header) + 3,
	                   G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL,
	                   "bad message destination rejected");
	alignas(g_message_header) uint8_t packet[sizeof(g_message_header) + 3];
	syscallTestRequire(g_receive_message_m(packet, sizeof(packet), G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) ==
	                   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL, "failed receive preserves message");
	auto message = (g_message_header*) packet;
	syscallTestRequire(!message->previous && !message->next && !memcmp(G_MESSAGE_CONTENT(message), "abc", 3),
	                   "message header contains no kernel links");
	g_syscall_send_message send{};
	send.receiver = g_get_tid(); send.buffer = (void*) "abc"; send.length = (1ULL << 32) + 3;
	g_syscall(G_SYSCALL_MESSAGE_SEND, (g_address) &send);
	syscallTestRequire(send.status == G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM, "message length checked before narrowing");

	g_syscall_fs_open open{};
	open.path = (char*) 0xffff800000001000ULL;
	g_syscall(G_SYSCALL_FS_OPEN, (g_address) &open);
	syscallTestRequire(open.status == G_FS_OPEN_ERROR, "invalid nested path recovers");
	g_kernquery_task_list_data list{};
	list.id_buffer = (g_tid*) 0xffff800000001000ULL; list.id_buffer_size = 8;
	syscallTestRequire(g_kernquery(G_KERNQUERY_TASK_LIST, (uint8_t*) &list) == G_KERNQUERY_STATUS_ERROR,
	                   "nested task ID array checked");
	g_syscall_log log{(char*) 0xffff800000001000ULL};
	g_syscall(G_SYSCALL_LOG, (g_address) &log);
	g_vm86_registers registers{};
	g_kernquery_task_security_data security{};
	security.id = g_get_tid();
	syscallTestRequire(g_kernquery(G_KERNQUERY_TASK_GET_SECURITY, (uint8_t*) &security) == G_KERNQUERY_STATUS_SUCCESSFUL,
	                   "query caller privilege");
	auto vm86Status = security.security_level > G_SECURITY_LEVEL_DRIVER
			? G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED : G_VM86_CALL_STATUS_NOT_SUPPORTED;
	syscallTestRequire(g_call_vm86(0x10, &registers, &registers) == vm86Status,
	                   "unsupported VM86 recovers");

	uint64_t rsdp = 0;
	uint8_t ioapic = 0;
	auto acpiStatus = g_acpi_get_platform_info(&rsdp, &ioapic);
	if(security.security_level > G_SECURITY_LEVEL_DRIVER)
	{
		syscallTestRequire(acpiStatus == G_ACPI_NOT_PERMITTED && !rsdp && !ioapic,
		                   "ACPI info requires driver privilege");
		syscallTestRequire(!g_acpi_map(0x1000, G_PAGE_SIZE), "ACPI mapping requires driver privilege");
	}
	else
	{
		syscallTestRequire(acpiStatus == G_ACPI_SUCCESSFUL && rsdp, "driver receives ACPI info");
		auto firmware = g_acpi_map(rsdp, 8);
		syscallTestRequire(firmware && !memcmp(firmware, "RSD PTR ", 8), "driver maps ACPI firmware");
		g_unmap((void*) ((g_address) firmware & ~(g_address) (G_PAGE_SIZE - 1)));

		g_syscall_acpi_map mapping{rsdp, 0, 123, G_ACPI_SUCCESSFUL};
		g_syscall(G_SYSCALL_ACPI_MAP, (g_address) &mapping);
		syscallTestRequire(mapping.status == G_ACPI_INVALID_ARGUMENT && !mapping.virtualAddress,
		                   "empty ACPI mapping rejected");
	}

	g_unmap(page);
	klog("syscall-memory-test: PASS all requests/nested pointers/pipe/IPC/recovery");
	return 0;
}
