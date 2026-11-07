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

namespace
{
#define GHOST_API_STDINT
using g_bool = uint8_t;
#include <ghost/messages/callstructs.h>
#include "support/memory/page_tables.hpp"
#include "production/memory_user_hpp.inc"

struct g_process { g_mutex lock{}; } process;
struct g_task { g_process* process; g_user_memory* userMemory; int securityLevel; g_tid id; } task{&process, nullptr, 2, 1};
#define G_PAGE_ALIGN_DOWN(value) ((value) & ~G_PAGE_ALIGN_MASK)
std::map<g_address, int> references;
bool failAllocation, resolveDemand;
unsigned demandFaults;
alignas(4096) uint8_t original[4096], replacement[4096], second[4096];
alignas(4096) uint64_t root[512];

void* heapTryAllocate(size_t length) {
	return failAllocation ? nullptr : malloc(length);
}
void heapFree(void* memory) { ASSERT_NE(nullptr, memory); free(memory); }
void pageReferenceTrackerIncrement(g_address physical) { ++references[physical]; }
void memoryPhysicalFree(g_address physical) { ASSERT_GT(references[physical], 0); --references[physical]; }
bool taskingMemoryHandleStackOverflow(g_task*, g_address) { return false; }
bool memoryOnDemandHandlePageFault(g_task*, g_address address)
{
	++demandFaults;
	return resolveDemand && pagingMapPage(G_PAGE_ALIGN_DOWN(address), (g_address) second, 7, 7);
}
bool memoryCopyFromUser(g_task*, void*, g_address, size_t);
bool memoryCopyToUser(g_task*, g_address, const void*, size_t);
#include "production/memory_user_cpp.inc"
#include "support/syscall_dispatch.hpp"

class UserMemory : public testing::Test
{
protected:
	void SetUp() override
	{
		ASSERT_EQ(nullptr, task.userMemory);
		for(auto entry : references) EXPECT_EQ(0, entry.second);
		references.clear();
		memset(root, 0, sizeof(root));
		memset(original, 0, sizeof(original));
		memset(replacement, 0, sizeof(replacement));
		nextPage = 0;
		currentSpace = (g_address) root;
		failAllocation = resolveDemand = false;
		task.securityLevel = G_SECURITY_LEVEL_APPLICATION;
		demandFaults = 0;
		ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 7));
	}
	void TearDown() override
	{
		memoryUserReleaseAll(&task);
		EXPECT_EQ(nullptr, task.userMemory);
		for(auto entry : references) EXPECT_EQ(0, entry.second);
	}
};

TEST_F(UserMemory, RetainsBackingAcrossUnmapAndRemap)
{
	auto output = memoryUserAcquire(&task, 0x400ffb, 5, true);
	ASSERT_TRUE(output);
	EXPECT_EQ(1, references[(g_address) original]);
	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) replacement, 7, 7));
	EXPECT_TRUE(memoryUserWrite(output, "hello", 5));
	EXPECT_EQ(0, memcmp(original + 4091, "hello", 5));
	EXPECT_EQ(0, replacement[4091]);
}

TEST_F(UserMemory, RejectsInvalidAndReadOnlyDestinationsBeforeRetainingAnything)
{
	EXPECT_EQ(nullptr, memoryUserAcquire(&task, 0xffff800000000000ULL, 1, true));
	EXPECT_EQ(nullptr, memoryUserAcquire(&task, G_MEM_LOWER_HALF_END, 2, true));
	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 5));
	EXPECT_EQ(nullptr, memoryUserAcquire(&task, 0x400000, 1, true));
	auto input = memoryUserAcquire(&task, 0x400000, 1, false);
	EXPECT_TRUE(input);
	EXPECT_FALSE(memoryUserWrite(input, "x", 1));
}

TEST_F(UserMemory, RollsBackAPrefixAndResolvesDemandPages)
{
	EXPECT_EQ(nullptr, memoryUserAcquire(&task, 0x400fff, 2, true));
	EXPECT_EQ(nullptr, task.userMemory);
	EXPECT_EQ(0, references[(g_address) original]);
	resolveDemand = true;
	auto output = memoryUserAcquire(&task, 0x400fff, 2, true);
	ASSERT_TRUE(output);
	EXPECT_TRUE(memoryUserWrite(output, "ab", 2));
	EXPECT_EQ('a', original[4095]);
	EXPECT_EQ('b', second[0]);
}

TEST_F(UserMemory, RespectsPermissionsAtEveryPageTableLevel)
{
	root[0] &= ~G_PAGE_USER_FLAG;
	uint8_t value;
	EXPECT_FALSE(memoryCopyFromUser(&task, &value, 0x400000, 1));
	EXPECT_EQ(nullptr, memoryUserAcquire(&task, 0x400000, 1, true));
	root[0] |= G_PAGE_USER_FLAG;
	root[0] &= ~G_PAGE_WRITABLE_FLAG;
	EXPECT_FALSE(memoryCopyToUser(&task, 0x400000, "x", 1));
	EXPECT_TRUE(memoryCopyFromUser(&task, &value, 0x400000, 1));
}

TEST_F(UserMemory, ReclaimsPinsWhenATaskIsCancelled)
{
	ASSERT_NE(nullptr, memoryUserAcquire(&task, 0x400000, 8, true));
	ASSERT_NE(nullptr, memoryUserAcquire(&task, 0x400010, 8, true));
	EXPECT_EQ(2, references[(g_address) original]);
	memoryUserReleaseAll(&task);
	EXPECT_EQ(nullptr, task.userMemory);
	EXPECT_EQ(0, references[(g_address) original]);
}

struct Request { uint32_t input; uint32_t output; };
unsigned operations;
void userMemoryOperation(g_task*, void* address)
{
	auto request = (Request*) address;
	++operations;
	pagingUnmapPage(0x400000);
	EXPECT_TRUE(pagingMapPage(0x400000, (g_address) replacement, 7, 7));
	request->output = request->input + 1;
}

TEST_F(UserMemory, SyscallAdapterSnapshotsInputsAndCompletesThePinnedReply)
{
	Request request{41, 0};
	memcpy(original, &request, sizeof(request));
	operations = 0;
	g_syscall_registration registration{userMemoryOperation, false, sizeof(Request), G_SYSCALL_IN_OUT};
	_syscallInvoke(&task, &registration, (void*) 0x400000);
	EXPECT_EQ(1U, operations);
	memcpy(&request, original, sizeof(request));
	EXPECT_EQ(42U, request.output);
	EXPECT_EQ(0, replacement[4]);
}

TEST_F(UserMemory, SyscallAdapterRejectsAnUndeliverableReplyBeforeTheOperation)
{
	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 5));
	operations = 0;
	g_syscall_registration registration{userMemoryOperation, false, sizeof(Request), G_SYSCALL_IN_OUT};
	_syscallInvoke(&task, &registration, (void*) 0x400000);
	EXPECT_EQ(0U, operations);
	failAllocation = true;
	_syscallInvoke(&task, &registration, (void*) 0x400000);
	EXPECT_EQ(0U, operations);
}

uint32_t expectedInput;
void userMemoryInspectRequest(g_task*, void* address)
{
	auto request = (Request*) address;
	EXPECT_EQ(expectedInput, request->input);
	request->output = 77;
	++operations;
}

TEST_F(UserMemory, SyscallAccessModeControlsInputAndOutputCopies)
{
	Request request{41, 0};
	memcpy(original, &request, sizeof(request));
	original[sizeof(request)] = 0xcc;
	operations = 0;
	expectedInput = 41;
	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 5));
	g_syscall_registration registration{userMemoryInspectRequest, false, sizeof(Request), G_SYSCALL_INPUT};
	_syscallInvoke(&task, &registration, (void*) 0x400000);
	memcpy(&request, original, sizeof(request));
	EXPECT_EQ(0U, request.output);
	EXPECT_EQ(1U, operations);
	EXPECT_EQ(nullptr, task.userMemory);

	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 7));
	expectedInput = 0;
	registration.access = G_SYSCALL_OUTPUT;
	_syscallInvoke(&task, &registration, (void*) 0x400000);
	memcpy(&request, original, sizeof(request));
	EXPECT_EQ(0U, request.input);
	EXPECT_EQ(77U, request.output);
	EXPECT_EQ(0xcc, original[sizeof(request)]);
	EXPECT_EQ(2U, operations);
}

TEST_F(UserMemory, RequestReplyCanEndAtTheLastUserAddress)
{
	auto page = G_PAGE_ALIGN_DOWN(G_MEM_LOWER_HALF_END);
	ASSERT_TRUE(pagingMapPage(page, (g_address) original, 7, 7));
	Request request{41, 0};
	memcpy(original + G_PAGE_SIZE - sizeof(request), &request, sizeof(request));
	expectedInput = 41;
	operations = 0;
	g_syscall_registration registration{userMemoryInspectRequest, false, sizeof(request), G_SYSCALL_IN_OUT};
	_syscallInvoke(&task, &registration, (void*) (G_MEM_LOWER_HALF_END - sizeof(request) + 1));
	memcpy(&request, original + G_PAGE_SIZE - sizeof(request), sizeof(request));
	EXPECT_EQ(1U, operations);
	EXPECT_EQ(77U, request.output);
}

unsigned sends, receives;
g_message_send_status messageQueueSend(g_tid, g_tid, void* bytes, uint32_t length, g_message_transaction)
{
	++sends;
	EXPECT_EQ(3U, length);
	EXPECT_EQ(0, memcmp(bytes, "abc", 3));
	return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}
bool messageQueueWaitForSend(g_task*, g_tid, uint32_t) { return false; }
void messageQueueUnwaitForSend(g_tid, g_tid) {}
g_message_receive_status messageQueueReceive(g_tid, g_message_header* message, uint32_t maximum, g_message_transaction)
{
	++receives;
	if(maximum < sizeof(*message) + 3) return G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
	*message = {7, 9, 3, (g_message_header*) 0xffff800000001000ULL, (g_message_header*) 0xffff800000002000ULL};
	memcpy(G_MESSAGE_CONTENT(message), "abc", 3);
	return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
}
bool messageQueueWaitForReceive(g_task*, g_message_transaction) { return false; }
g_message_transaction messageQueueNextTxId() { return 9; }
g_message_send_status messageTopicsPost(const char*, g_tid, void*, uint32_t) { return G_MESSAGE_SEND_STATUS_SUCCESSFUL; }
g_message_receive_status messageTopicsReceive(const char*, g_message_transaction, void*, uint32_t) { return G_MESSAGE_RECEIVE_STATUS_EMPTY; }
bool messageTopicsWaitForReceive(const char*, g_task*, g_message_transaction) { return false; }
void messageTopicsUnwaitForReceive(const char*, g_tid) {}
template<typename F> bool taskingWaitIf(F function) { return function(); }
#include "production/syscall_messages.inc"

TEST_F(UserMemory, MessagingChecksWideLengthsAndSanitizesHeaderLinks)
{
	sends = receives = 0;
	memcpy(original, "abc", 3);
	g_syscall_send_message send{};
	send.buffer = (void*) 0x400000;
	send.length = (1ULL << 32) + 3;
	syscallMessageSend(&task, &send);
	EXPECT_EQ(G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM, send.status);
	EXPECT_EQ(0U, sends);
	send.length = 3;
	syscallMessageSend(&task, &send);
	EXPECT_EQ(1U, sends);
	g_syscall_receive_message receive{};
	receive.buffer = (g_message_header*) 0x400000;
	receive.maximum = sizeof(g_message_header) + 3;
	syscallMessageReceive(&task, &receive);
	EXPECT_EQ(G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL, receive.status);
	g_message_header message;
	memcpy(&message, original, sizeof(message));
	EXPECT_EQ(nullptr, message.previous);
	EXPECT_EQ(nullptr, message.next);
	EXPECT_EQ(0, memcmp(original + sizeof(message), "abc", 3));
	pagingUnmapPage(0x400000);
	ASSERT_TRUE(pagingMapPage(0x400000, (g_address) original, 7, 5));
	syscallMessageReceive(&task, &receive);
	EXPECT_EQ(1U, receives);
}
}
