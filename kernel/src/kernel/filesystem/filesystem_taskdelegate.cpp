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

#include "kernel/filesystem/filesystem_taskdelegate.hpp"
#include "kernel/ipc/message_queues.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/utils/string.hpp"

static constexpr uint64_t G_FS_DELEGATE_TIMEOUT_MS = 15000;
static constexpr uint32_t G_FS_DELEGATE_MAX_ACTIVE = 64;
static constexpr uint32_t G_FS_DELEGATE_MAX_RECORDS = 1024;
static constexpr uint32_t G_FS_DELEGATE_MAX_NODES = 16384;

static g_mutex filesystemDelegateLock;
static g_tid filesystemDelegateDrainOwner = G_TID_NONE;
static g_fs_task_delegate* filesystemDelegates;
static g_fs_transaction_id filesystemDelegateNextTransaction;
static uint32_t filesystemDelegateActiveCount;
static uint32_t filesystemDelegateRecordCount;
static uint32_t filesystemDelegateNodeCount;

static g_fs_open_status filesystemDelegateOpen(g_fs_node* node, g_file_flag_mode flags);
static g_fs_close_status filesystemDelegateClose(g_fs_node* node, g_file_flag_mode flags);
static g_fs_open_status filesystemDelegateLookup(g_fs_node* node, const char* name, g_fs_node** out);
static g_fs_read_status filesystemDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
											   int64_t* out);
static g_fs_write_status filesystemDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
												 int64_t* out);
static g_fs_length_status filesystemDelegateGetLength(g_fs_node* node, uint64_t* out);
static g_fs_directory_refresh_status filesystemDelegateRefresh(g_fs_node* node);

void filesystemDelegateInitialize()
{
	mutexInitializeTask(&filesystemDelegateLock, __func__);
	filesystemDelegateDrainOwner = G_TID_NONE;
}

static void filesystemDelegateDiscardPending(g_fs_delegate_transaction* transaction)
{
	while(transaction->pending)
	{
		auto pending = transaction->pending;
		transaction->pending = pending->next;

		filesystemDiscardDetachedNode(pending->node);

		heapFree(pending->entry);
		heapFree(pending);

		--filesystemDelegateNodeCount;
	}
}

/**
 * Published VFS nodes have the kernel's existing persistent cache lifetime.
 * Retired records remain as bounded tombstones so old descriptors/iterators
 * cannot attach to a new mount or dereference freed delegate callbacks.
 */
static void filesystemDelegateDetach(g_fs_task_delegate* delegate)
{
	if(!delegate->online)
		return;

	delegate->online = false;

	--filesystemDelegateActiveCount;
	if(delegate->mountEntry)
		filesystemDetachNode(delegate->root, delegate->mountEntry);
	delegate->mountEntry = nullptr;

	auto transaction = delegate->transaction;
	delegate->transaction = nullptr;
	if(transaction)
	{
		filesystemDelegateDiscardPending(transaction);
		transaction->cancelled = true;
		transaction->completed = true;
		if(transaction->client == G_TID_NONE)
			heapFree(transaction);
	}

	waitQueueWake(&delegate->waiters);
}

static bool filesystemDelegateOwnerAlive(g_fs_task_delegate* delegate)
{
	auto owner = taskingAcquireTask(delegate->owner);
	bool alive = owner && !memoryAtomicLoad(&owner->terminationRequested, G_MEMORY_ORDER_ACQUIRE);
	taskingReleaseTask(owner);
	return alive;
}

bool filesystemDelegateAvailable(g_fs_node* node)
{
	if(memoryAtomicLoad(&node->removed, G_MEMORY_ORDER_ACQUIRE))
		return false;

	if(!node->delegate || node->delegate->open != filesystemDelegateOpen)
		return true;

	auto delegate = (g_fs_task_delegate*) node->delegate;
	g_mutex_guard guard(&filesystemDelegateLock);

	if(delegate->online && !filesystemDelegateOwnerAlive(delegate))
		filesystemDelegateDetach(delegate);

	return delegate->online;
}

/**
 * Delegate callbacks can run with a descriptor pin and task mutex held. Defer
 * termination until the callback unwinds; taskingWaitIf would abandon these
 * resources at its cancellation boundary. No delegate lock crosses the yield.
 */
static void filesystemDelegateWait(g_fs_task_delegate* delegate, g_task* task, uint64_t deadline)
{
	waitQueueAdd(&delegate->waiters, task->id);

	taskingPrepareWait(task, "filesystem-delegate");
	auto poll = clockGetMilliseconds() + 10;
	clockWaitForTime(task->id, poll < deadline ? poll : deadline);

	mutexRelease(&filesystemDelegateLock);
	taskingYield();
	clockUnwaitForTime(task);
	mutexAcquire(&filesystemDelegateLock);

	waitQueueRemove(&delegate->waiters, task->id);
}

static void filesystemDelegatePrepare(g_fs_delegate_transaction_storage* storage, g_fs_delegate_request_type type)
{
	memorySetBytes(storage, 0, sizeof(*storage));
	storage->header.type = type;
}

/**
 * Transaction helpers run under filesystemDelegateLock. Waiting temporarily
 * releases that lock, then reacquires it before inspecting delegate state.
 */
static bool filesystemDelegateAcquireSlot(g_fs_task_delegate* delegate, g_task* task, uint64_t deadline)
{
	while(delegate->online && delegate->transaction && clockGetMilliseconds() < deadline &&
		  !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE))
	{
		if(!filesystemDelegateOwnerAlive(delegate))
		{
			filesystemDelegateDetach(delegate);
			break;
		}

		filesystemDelegateWait(delegate, task, deadline);
	}

	if(delegate->online && !filesystemDelegateOwnerAlive(delegate))
		filesystemDelegateDetach(delegate);

	if(!delegate->online || delegate->transaction || clockGetMilliseconds() >= deadline ||
	   memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE))
		return false;

	return true;
}

static g_fs_delegate_transaction* filesystemDelegateBeginTransaction(g_fs_task_delegate* delegate, g_fs_node* node,
																	 g_fs_delegate_transaction_storage* storage,
																	 uint8_t* buffer, g_task* task)
{
	auto transaction = static_cast<g_fs_delegate_transaction*>(heapTryAllocate(sizeof(g_fs_delegate_transaction)));
	if(!transaction)
	{
		return nullptr;
	}

	memorySetBytes(transaction, 0, sizeof(*transaction));
	if(++filesystemDelegateNextTransaction == G_FS_TRANSACTION_NONE)
		++filesystemDelegateNextTransaction;

	storage->header.version = G_FS_DELEGATE_VERSION;
	storage->header.storage_size = sizeof(*storage);
	storage->header.transaction = filesystemDelegateNextTransaction;
	storage->header.mountpoint_id = delegate->root->id;

	auto io = (void*) G_MEM_PHYS_TO_VIRT(delegate->ioPage);
	memorySetBytes(io, 0, G_PAGE_SIZE);

	if(storage->header.type == G_FS_DELEGATE_REQUEST_READ)
		storage->data.read.mapped_buffer = (void*) (delegate->mapping + G_PAGE_SIZE);

	if(storage->header.type == G_FS_DELEGATE_REQUEST_WRITE)
	{
		storage->data.write.mapped_buffer = (void*) (delegate->mapping + G_PAGE_SIZE);
		memoryCopy(io, buffer, storage->data.write.length);
	}

	transaction->request = *storage;
	transaction->node = node;
	transaction->buffer = buffer;
	transaction->client = task->id;
	delegate->transaction = transaction;

	auto control = (void*) G_MEM_PHYS_TO_VIRT(delegate->controlPage);
	memorySetBytes(control, 0, G_PAGE_SIZE);
	memoryCopy(control, storage, sizeof(*storage));
	memoryAtomicFence(G_MEMORY_ORDER_RELEASE);

	auto sent = messageQueueSend(G_FS_DELEGATE_MESSAGE_SENDER, delegate->owner, &storage->header,
								 sizeof(storage->header), G_MESSAGE_TRANSACTION_NONE);
	if(sent != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		delegate->transaction = nullptr;
		heapFree(transaction);
		return nullptr;
	}

	return transaction;
}

static bool filesystemDelegateAwaitResult(g_fs_task_delegate* delegate, g_fs_delegate_transaction* transaction,
										  g_fs_delegate_transaction_storage* storage, g_task* task, uint64_t deadline)
{
	while(!transaction->completed && clockGetMilliseconds() < deadline &&
		  !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE))
	{
		if(!filesystemDelegateOwnerAlive(delegate))
		{
			filesystemDelegateDetach(delegate);
			break;
		}

		filesystemDelegateWait(delegate, task, deadline);
	}

	bool successful = transaction->completed && !transaction->cancelled &&
					  !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE);
	if(transaction->completed)
	{
		if(successful)
			*storage = transaction->result;
		heapFree(transaction);
	}
	else
	{
		transaction->cancelled = true;
		transaction->client = G_TID_NONE;
		transaction->buffer = nullptr;
		filesystemDelegateDiscardPending(transaction);
	}

	return successful;
}

static bool filesystemDelegateTransact(g_fs_node* node, g_fs_delegate_transaction_storage* storage,
									   uint8_t* buffer = nullptr)
{
	auto task = taskingGetCurrentTask();
	auto delegate = (g_fs_task_delegate*) node->delegate;
	if(task->id == delegate->owner)
		return false;

	auto deadline = clockGetMilliseconds() + G_FS_DELEGATE_TIMEOUT_MS;
	g_interrupt_guard interrupts;
	g_mutex_guard guard(&filesystemDelegateLock);

	if(filesystemDelegateDrainOwner != G_TID_NONE && task->id != filesystemDelegateDrainOwner)
		return false;

	if(!filesystemDelegateAcquireSlot(delegate, task, deadline))
		return false;

	auto transaction = filesystemDelegateBeginTransaction(delegate, node, storage, buffer, task);
	if(!transaction)
		return false;

	return filesystemDelegateAwaitResult(delegate, transaction, storage, task, deadline);
}

static g_fs_open_status filesystemDelegateOpen(g_fs_node* node, g_file_flag_mode flags)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_OPEN);
	storage.data.open.phys_fs_id = node->physicalId;
	storage.data.open.result_status = G_FS_OPEN_ERROR;
	{
		g_mutex_guard nodeGuard(&node->lock);
		stringCopy(storage.data.open.name, node->name);
	}
	storage.data.open.flags = flags;
	return filesystemDelegateTransact(node, &storage) ? storage.data.open.result_status : G_FS_OPEN_ERROR;
}

static g_fs_close_status filesystemDelegateClose(g_fs_node* node, g_file_flag_mode flags)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_CLOSE);
	storage.data.close.phys_fs_id = node->physicalId;
	storage.data.close.result_status = G_FS_CLOSE_ERROR;
	storage.data.close.flags = flags;
	return filesystemDelegateTransact(node, &storage) ? storage.data.close.result_status : G_FS_CLOSE_ERROR;
}

static g_fs_open_status filesystemDelegateLookup(g_fs_node* node, const char* name, g_fs_node** out)
{
	*out = nullptr;
	if(node->type != G_FS_NODE_TYPE_FOLDER && node->type != G_FS_NODE_TYPE_MOUNTPOINT &&
	   node->type != G_FS_NODE_TYPE_ROOT)
		return G_FS_OPEN_NOT_FOUND;
	if(stringLength(name) >= G_FILENAME_MAX)
		return G_FS_OPEN_ERROR;

	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_LOOKUP);
	storage.data.lookup.parent_phys_fs_id = node->physicalId;
	storage.data.lookup.parent_virt_fs_id = node->id;
	storage.data.lookup.result_node_id = G_FS_VIRT_ID_NONE;
	storage.data.lookup.result_status = G_FS_LOOKUP_ERROR;
	stringCopy(storage.data.lookup.name, name);
	if(!filesystemDelegateTransact(node, &storage))
		return G_FS_OPEN_ERROR;

	switch(storage.data.lookup.result_status)
	{
		case G_FS_LOOKUP_SUCCESSFUL:
			*out = filesystemGetNode(storage.data.lookup.result_node_id);
			return *out ? G_FS_OPEN_SUCCESSFUL : G_FS_OPEN_ERROR;
		case G_FS_LOOKUP_NOT_FOUND:
			return G_FS_OPEN_NOT_FOUND;
		case G_FS_LOOKUP_BUSY:
			return G_FS_OPEN_BUSY;
		default:
			return G_FS_OPEN_ERROR;
	}
}

static g_fs_read_status filesystemDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
											   int64_t* out)
{
	*out = -1;
	if(!length)
	{
		*out = 0;
		return G_FS_READ_SUCCESSFUL;
	}

	uint32_t count = length < G_PAGE_SIZE ? length : G_PAGE_SIZE;
	if(offset > UINT64_MAX - count)
		return G_FS_READ_ERROR;

	auto bytes = static_cast<uint8_t*>(heapTryAllocate(count));
	if(!bytes)
		return G_FS_READ_ERROR;

	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_READ);
	storage.data.read.phys_fs_id = node->physicalId;
	storage.data.read.result_read = -1;
	storage.data.read.result_status = G_FS_READ_ERROR;
	storage.data.read.offset = offset;
	storage.data.read.length = count;
	auto status = G_FS_READ_ERROR;
	if(filesystemDelegateTransact(node, &storage, bytes))
	{
		status = storage.data.read.result_status;
		if(status == G_FS_READ_SUCCESSFUL)
		{
			if(filesystemDelegateCopyToCaller(taskingGetCurrentTask(), buffer, bytes, storage.data.read.result_read))
				*out = storage.data.read.result_read;
			else
				status = G_FS_READ_ERROR;
		}
	}

	heapFree(bytes);
	return status;
}

static g_fs_write_status filesystemDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
												 int64_t* out)
{
	*out = -1;
	if(!length)
	{
		*out = 0;
		return G_FS_WRITE_SUCCESSFUL;
	}

	uint32_t count = length < G_PAGE_SIZE ? length : G_PAGE_SIZE;
	if(offset > UINT64_MAX - count)
		return G_FS_WRITE_ERROR;

	auto bytes = static_cast<uint8_t*>(heapTryAllocate(count));
	if(!bytes)
		return G_FS_WRITE_ERROR;

	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_WRITE);
	storage.data.write.phys_fs_id = node->physicalId;
	storage.data.write.result_write = -1;
	storage.data.write.result_status = G_FS_WRITE_ERROR;
	storage.data.write.offset = offset;
	storage.data.write.length = count;
	auto status = G_FS_WRITE_ERROR;
	if(filesystemDelegateCopyFromCaller(taskingGetCurrentTask(), bytes, buffer, count) &&
	   filesystemDelegateTransact(node, &storage, bytes))
	{
		status = storage.data.write.result_status;
		if(status == G_FS_WRITE_SUCCESSFUL)
			*out = storage.data.write.result_write;
	}

	heapFree(bytes);
	return status;
}

static g_fs_length_status filesystemDelegateGetLength(g_fs_node* node, uint64_t* out)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_GET_LENGTH);
	storage.data.get_length.phys_fs_id = node->physicalId;
	storage.data.get_length.result_status = G_FS_LENGTH_ERROR;
	if(!filesystemDelegateTransact(node, &storage))
		return G_FS_LENGTH_ERROR;
	if(storage.data.get_length.result_status == G_FS_LENGTH_SUCCESSFUL)
		*out = storage.data.get_length.result_length;
	return storage.data.get_length.result_status;
}

static g_fs_directory_refresh_status filesystemDelegateRefresh(g_fs_node* node)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_READ_DIRECTORY);
	storage.data.directory_refresh.parent_phys_fs_id = node->physicalId;
	storage.data.directory_refresh.parent_virt_fs_id = node->id;
	storage.data.directory_refresh.result_status = G_FS_DIRECTORY_REFRESH_ERROR;
	return filesystemDelegateTransact(node, &storage) ? storage.data.directory_refresh.result_status
													  : G_FS_DIRECTORY_REFRESH_ERROR;
}

static g_fs_mutation_status filesystemDelegateCreateEntry(g_fs_node* parent, const char* name, g_fs_node_type type,
														  g_fs_node** out)
{
	*out = nullptr;
	if(stringLength(name) >= G_FILENAME_MAX)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_CREATE);
	storage.data.create.parent_phys_fs_id = parent->physicalId;
	storage.data.create.parent_virt_fs_id = parent->id;
	storage.data.create.result_node_id = G_FS_VIRT_ID_NONE;
	storage.data.create.result_status = G_FS_MUTATION_ERROR;
	storage.data.create.type = type;
	stringCopy(storage.data.create.name, name);
	if(!filesystemDelegateTransact(parent, &storage))
		return G_FS_MUTATION_ERROR;
	if(storage.data.create.result_status == G_FS_MUTATION_SUCCESSFUL)
		*out = filesystemGetNode(storage.data.create.result_node_id);
	return storage.data.create.result_status;
}

static g_fs_open_status filesystemDelegateCreate(g_fs_node* parent, const char* name, g_fs_node** out)
{
	auto status = filesystemDelegateCreateEntry(parent, name, G_FS_NODE_TYPE_FILE, out);
	if(status == G_FS_MUTATION_SUCCESSFUL) return G_FS_OPEN_SUCCESSFUL;
	return status == G_FS_MUTATION_ALREADY_EXISTS ? G_FS_OPEN_ALREADY_EXISTS : G_FS_OPEN_ERROR;
}

static g_fs_mutation_status filesystemDelegateMakeDirectory(g_fs_node* parent, const char* name, g_fs_node** out)
{
	return filesystemDelegateCreateEntry(parent, name, G_FS_NODE_TYPE_FOLDER, out);
}

static g_fs_mutation_status filesystemDelegateRename(g_fs_node* node, g_fs_node* parent, const char* name,
													g_fs_node* replaced)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, G_FS_DELEGATE_REQUEST_RENAME);
	storage.data.rename.phys_fs_id = node->physicalId;
	storage.data.rename.parent_phys_fs_id = parent->physicalId;
	storage.data.rename.replaced_phys_fs_id = replaced ? replaced->physicalId : UINT64_MAX;
	stringCopy(storage.data.rename.name, name);
	storage.data.rename.result_status = G_FS_MUTATION_ERROR;
	if(!filesystemDelegateTransact(node, &storage))
		return G_FS_MUTATION_ERROR;

	return storage.data.rename.result_status;
}

static g_fs_mutation_status filesystemDelegateMutate(g_fs_node* node, g_fs_delegate_request_type type, uint64_t length)
{
	g_fs_delegate_transaction_storage storage;
	filesystemDelegatePrepare(&storage, type);
	switch(type)
	{
		case G_FS_DELEGATE_REQUEST_RESIZE:
			storage.data.resize.phys_fs_id = node->physicalId;
			storage.data.resize.length = length;
			storage.data.resize.result_status = G_FS_MUTATION_ERROR;
			break;
		case G_FS_DELEGATE_REQUEST_REMOVE:
			storage.data.remove.phys_fs_id = node->physicalId;
			storage.data.remove.type = node->type;
			storage.data.remove.result_status = G_FS_MUTATION_ERROR;
			break;
		case G_FS_DELEGATE_REQUEST_FLUSH:
			storage.data.flush.phys_fs_id = node->physicalId;
			storage.data.flush.result_status = G_FS_MUTATION_ERROR;
			break;
		default:
			return G_FS_MUTATION_NOT_SUPPORTED;
	}

	if(!filesystemDelegateTransact(node, &storage))
		return G_FS_MUTATION_ERROR;

	switch(type)
	{
		case G_FS_DELEGATE_REQUEST_RESIZE:
			return storage.data.resize.result_status;
		case G_FS_DELEGATE_REQUEST_REMOVE:
			return storage.data.remove.result_status;
		case G_FS_DELEGATE_REQUEST_FLUSH:
			return storage.data.flush.result_status;
		default:
			return G_FS_MUTATION_NOT_SUPPORTED;
	}
}

bool filesystemDelegateFlushAll(g_tid coordinator)
{
	g_fs_node* roots[G_FS_DELEGATE_MAX_ACTIVE];
	uint32_t count = 0;

	mutexAcquire(&filesystemDelegateLock);
	filesystemDelegateDrainOwner = coordinator;
	for(auto delegate = filesystemDelegates; delegate; delegate = delegate->next)
	{
		if(delegate->online)
		{
			roots[count++] = delegate->root;
		}
	}

	mutexRelease(&filesystemDelegateLock);

	for(uint32_t i = 0; i < count; ++i)
	{
		if(filesystemDelegateMutate(roots[i], G_FS_DELEGATE_REQUEST_FLUSH, 0) != G_FS_MUTATION_SUCCESSFUL)
		{
			return false;
		}
	}

	return true;
}

void filesystemDelegateResume()
{
	g_mutex_guard guard(&filesystemDelegateLock);
	filesystemDelegateDrainOwner = G_TID_NONE;
}

static g_fs_open_status filesystemDelegateTruncate(g_fs_node* node)
{
	return filesystemDelegateMutate(node, G_FS_DELEGATE_REQUEST_RESIZE, 0) == G_FS_MUTATION_SUCCESSFUL
				   ? G_FS_OPEN_SUCCESSFUL
				   : G_FS_OPEN_ERROR;
}

static g_fs_task_delegate* filesystemDelegateFindMount(g_fs_virt_id mount)
{
	for(auto delegate = filesystemDelegates; delegate; delegate = delegate->next)
		if(delegate->root->id == mount)
			return delegate;

	return nullptr;
}

static g_fs_task_delegate* filesystemDelegateFindTransaction(g_fs_transaction_id id)
{
	for(auto delegate = filesystemDelegates; delegate; delegate = delegate->next)
		if(delegate->transaction && delegate->transaction->request.header.transaction == id)
			return delegate;

	return nullptr;
}

static g_fs_node* filesystemDelegateFindResultNode(g_fs_delegate_transaction* transaction, g_fs_virt_id id)
{
	for(auto pending = transaction->pending; pending; pending = pending->next)
		if(pending->node->id == id)
			return pending->node;

	return filesystemGetNode(id);
}

static size_t filesystemDelegateInputSize(g_fs_delegate_request_type type)
{
	switch(type)
	{
		case G_FS_DELEGATE_REQUEST_LOOKUP:
			return offsetof(g_fs_delegate_lookup, result_node_id);
		case G_FS_DELEGATE_REQUEST_READ:
			return offsetof(g_fs_delegate_read, result_read);
		case G_FS_DELEGATE_REQUEST_WRITE:
			return offsetof(g_fs_delegate_write, result_write);
		case G_FS_DELEGATE_REQUEST_GET_LENGTH:
			return offsetof(g_fs_delegate_get_length, result_length);
		case G_FS_DELEGATE_REQUEST_READ_DIRECTORY:
			return offsetof(g_fs_delegate_directory_refresh, result_status);
		case G_FS_DELEGATE_REQUEST_OPEN:
			return offsetof(g_fs_delegate_open, result_status);
		case G_FS_DELEGATE_REQUEST_CLOSE:
			return offsetof(g_fs_delegate_close, result_status);
		case G_FS_DELEGATE_REQUEST_CREATE:
			return offsetof(g_fs_delegate_create, result_node_id);
		case G_FS_DELEGATE_REQUEST_RESIZE:
			return offsetof(g_fs_delegate_resize, result_status);
		case G_FS_DELEGATE_REQUEST_REMOVE:
			return offsetof(g_fs_delegate_remove, result_status);
		case G_FS_DELEGATE_REQUEST_RENAME:
			return offsetof(g_fs_delegate_rename, result_status);
		case G_FS_DELEGATE_REQUEST_FLUSH:
			return offsetof(g_fs_delegate_flush, result_status);
		default:
			return 0;
	}
}

static bool filesystemDelegateValidateResult(g_fs_delegate_transaction* transaction,
											 const g_fs_delegate_transaction_storage& result)
{
	auto expected = (const uint8_t*) (&transaction->request);
	auto actual = (const uint8_t*) (&result);
	auto size = sizeof(result.header) + filesystemDelegateInputSize(transaction->request.header.type);
	for(size_t i = 0; i < size; ++i)
		if(expected[i] != actual[i])
			return false;

	switch(transaction->request.header.type)
	{
		case G_FS_DELEGATE_REQUEST_LOOKUP:
			if(result.data.lookup.result_status < 0 || result.data.lookup.result_status > G_FS_LOOKUP_ERROR)
				return false;
			if(result.data.lookup.result_status == G_FS_LOOKUP_SUCCESSFUL)
			{
				auto node = filesystemDelegateFindResultNode(transaction, result.data.lookup.result_node_id);
				return node && node->parent == transaction->node && node->delegate == transaction->node->delegate;
			}

			return true;
		case G_FS_DELEGATE_REQUEST_READ:
			return result.data.read.result_status >= 0 && result.data.read.result_status <= G_FS_READ_ERROR &&
				   (result.data.read.result_status != G_FS_READ_SUCCESSFUL ||
					(result.data.read.result_read >= 0 &&
					 uint64_t(result.data.read.result_read) <= transaction->request.data.read.length));
		case G_FS_DELEGATE_REQUEST_WRITE:
			return result.data.write.result_status >= 0 && result.data.write.result_status <= G_FS_WRITE_NO_SPACE &&
				   (result.data.write.result_status != G_FS_WRITE_SUCCESSFUL ||
					(result.data.write.result_write >= 0 &&
					 uint64_t(result.data.write.result_write) <= transaction->request.data.write.length));
		case G_FS_DELEGATE_REQUEST_GET_LENGTH:
			return result.data.get_length.result_status >= 0 &&
				   result.data.get_length.result_status <= G_FS_LENGTH_ERROR;
		case G_FS_DELEGATE_REQUEST_READ_DIRECTORY:
			return result.data.directory_refresh.result_status >= 0 &&
				   result.data.directory_refresh.result_status <= G_FS_DIRECTORY_REFRESH_BUSY;
		case G_FS_DELEGATE_REQUEST_OPEN:
			return result.data.open.result_status >= 0 && result.data.open.result_status <= G_FS_OPEN_BUSY;
		case G_FS_DELEGATE_REQUEST_CLOSE:
			return result.data.close.result_status >= 0 && result.data.close.result_status <= G_FS_CLOSE_ERROR;
		case G_FS_DELEGATE_REQUEST_CREATE:
			if(result.data.create.result_status < 0 || result.data.create.result_status > G_FS_MUTATION_ERROR)
				return false;
			if(result.data.create.result_status == G_FS_MUTATION_SUCCESSFUL)
			{
				auto node = filesystemDelegateFindResultNode(transaction, result.data.create.result_node_id);
				return node && node->parent == transaction->node && node->delegate == transaction->node->delegate &&
					   node->type == filesystemStorageNodeType(transaction->request.data.create.type, transaction->request.data.create.name);
			}

			return true;
		case G_FS_DELEGATE_REQUEST_RESIZE:
			return result.data.resize.result_status >= 0 && result.data.resize.result_status <= G_FS_MUTATION_ERROR;
		case G_FS_DELEGATE_REQUEST_REMOVE:
			return result.data.remove.result_status >= 0 && result.data.remove.result_status <= G_FS_MUTATION_ERROR;
		case G_FS_DELEGATE_REQUEST_RENAME:
			return result.data.rename.result_status >= 0 && result.data.rename.result_status <= G_FS_MUTATION_CROSS_DEVICE;
		case G_FS_DELEGATE_REQUEST_FLUSH:
			return result.data.flush.result_status >= 0 && result.data.flush.result_status <= G_FS_MUTATION_ERROR;
		default:
			return false;
	}
}

static void filesystemDelegatePublishResult(g_fs_delegate_transaction* transaction,
											const g_fs_delegate_transaction_storage& result)
{
	bool publish = (result.header.type == G_FS_DELEGATE_REQUEST_CREATE &&
					result.data.create.result_status == G_FS_MUTATION_SUCCESSFUL) ||
				   (result.header.type == G_FS_DELEGATE_REQUEST_LOOKUP &&
					result.data.lookup.result_status == G_FS_LOOKUP_SUCCESSFUL) ||
				   (result.header.type == G_FS_DELEGATE_REQUEST_READ_DIRECTORY &&
					result.data.directory_refresh.result_status == G_FS_DIRECTORY_REFRESH_SUCCESSFUL);
	while(publish && transaction->pending)
	{
		auto pending = transaction->pending;
		transaction->pending = pending->next;
		filesystemPublishNode(pending->node, pending->entry);
		heapFree(pending);
	}

	filesystemDelegateDiscardPending(transaction);
}

g_fs_delegate_status filesystemDelegateComplete(g_task* task, g_fs_transaction_id id)
{
	g_mutex_guard guard(&filesystemDelegateLock);
	auto delegate = filesystemDelegateFindTransaction(id);
	if(!delegate)
		return G_FS_DELEGATE_NOT_FOUND;
	if(delegate->owner != task->id)
		return G_FS_DELEGATE_NOT_OWNER;

	auto transaction = delegate->transaction;
	if(transaction->cancelled)
	{
		if(transaction->request.header.type == G_FS_DELEGATE_REQUEST_REMOVE)
			filesystemUnlinkNode(transaction->node);
		if(transaction->request.header.type == G_FS_DELEGATE_REQUEST_CREATE)
			transaction->node->upToDate = false;
		delegate->transaction = nullptr;
		heapFree(transaction);
		waitQueueWake(&delegate->waiters);
		return G_FS_DELEGATE_STALE_TRANSACTION;
	}

	g_fs_delegate_transaction_storage result;
	memoryAtomicFence(G_MEMORY_ORDER_ACQUIRE);
	memoryCopy(&result, (void*) G_MEM_PHYS_TO_VIRT(delegate->controlPage), sizeof(result));
	if(!filesystemDelegateValidateResult(transaction, result))
		return G_FS_DELEGATE_INVALID_RESULT;
	if(result.header.type == G_FS_DELEGATE_REQUEST_REMOVE &&
	   (result.data.remove.result_status == G_FS_MUTATION_SUCCESSFUL ||
		result.data.remove.result_status == G_FS_MUTATION_ERROR))
		filesystemUnlinkNode(transaction->node);
	if(result.header.type == G_FS_DELEGATE_REQUEST_CREATE)
		transaction->node->upToDate = false;
	filesystemDelegatePublishResult(transaction, result);
	if(result.header.type == G_FS_DELEGATE_REQUEST_READ && result.data.read.result_status == G_FS_READ_SUCCESSFUL)
		memoryCopy(transaction->buffer, (void*) G_MEM_PHYS_TO_VIRT(delegate->ioPage),
				   result.data.read.result_read);
	transaction->result = result;
	transaction->completed = true;
	delegate->transaction = nullptr;
	waitQueueWake(&delegate->waiters);
	return G_FS_DELEGATE_SUCCESSFUL;
}

static bool filesystemDelegateValidateNodeCreation(g_fs_delegate_transaction* transaction,
												   const g_syscall_fs_delegate_node_create* data)
{
	if((transaction->node->type != G_FS_NODE_TYPE_FOLDER && transaction->node->type != G_FS_NODE_TYPE_MOUNTPOINT &&
		transaction->node->type != G_FS_NODE_TYPE_ROOT) ||
	   data->parent_id != transaction->node->id ||
	   (transaction->request.header.type != G_FS_DELEGATE_REQUEST_LOOKUP &&
		transaction->request.header.type != G_FS_DELEGATE_REQUEST_READ_DIRECTORY &&
		transaction->request.header.type != G_FS_DELEGATE_REQUEST_CREATE) ||
	   (data->type != G_FS_NODE_TYPE_FILE && data->type != G_FS_NODE_TYPE_FOLDER && data->type != G_FS_NODE_TYPE_SYMLINK))
		return false;
	if(transaction->request.header.type == G_FS_DELEGATE_REQUEST_CREATE &&
	   data->type != transaction->request.data.create.type)
		return false;

	return true;
}

g_fs_delegate_status filesystemDelegateCreateNode(g_task* task, g_syscall_fs_delegate_node_create* data)
{
	g_mutex_guard guard(&filesystemDelegateLock);
	auto delegate = filesystemDelegateFindTransaction(data->transaction);
	if(!delegate)
		return G_FS_DELEGATE_NOT_FOUND;
	if(delegate->owner != task->id)
		return G_FS_DELEGATE_NOT_OWNER;

	auto transaction = delegate->transaction;
	if(transaction->cancelled)
		return G_FS_DELEGATE_STALE_TRANSACTION;
	if(!filesystemDelegateValidateNodeCreation(transaction, data))
		return G_FS_DELEGATE_INVALID_ARGUMENT;

	g_fs_node* existing = nullptr;
	filesystemFindExistingChild(transaction->node, data->name, &existing);
	for(auto pending = transaction->pending; !existing && pending; pending = pending->next)
		if(stringEquals(pending->node->name, data->name))
			existing = pending->node;
	if(existing)
	{
		if(existing->physicalId != data->phys_fs_id || existing->type != filesystemStorageNodeType(data->type, data->name))
			return G_FS_DELEGATE_INVALID_ARGUMENT;

		data->created_id = existing->id;
		return G_FS_DELEGATE_SUCCESSFUL;
	}

	if(filesystemDelegateNodeCount >= G_FS_DELEGATE_MAX_NODES)
		return G_FS_DELEGATE_NO_MEMORY;

	auto pending = static_cast<g_fs_delegate_pending_node*>(heapTryAllocate(sizeof(g_fs_delegate_pending_node)));
	auto entry = static_cast<g_fs_node_entry*>(heapTryAllocate(sizeof(g_fs_node_entry)));
	auto node = filesystemCreateDetachedNode(data->type, data->name);
	if(!pending || !entry || !node)
	{
		if(pending)
			heapFree(pending);
		if(entry)
			heapFree(entry);
		if(node)
			filesystemDiscardDetachedNode(node);
		return G_FS_DELEGATE_NO_MEMORY;
	}

	node->physicalId = data->phys_fs_id;
	node->parent = transaction->node;
	node->delegate = &delegate->base;
	*pending = {node, entry, transaction->pending};
	transaction->pending = pending;
	++filesystemDelegateNodeCount;
	data->created_id = node->id;
	return G_FS_DELEGATE_SUCCESSFUL;
}

static void filesystemDelegateInitializeCallbacks(g_fs_task_delegate* delegate)
{
	mutexInitializeTask(&delegate->base.lock, __func__);
	waitQueueInitialize(&delegate->waiters);
	delegate->base.open = filesystemDelegateOpen;
	delegate->base.close = filesystemDelegateClose;
	delegate->base.lookup = filesystemDelegateLookup;
	delegate->base.read = filesystemDelegateRead;
	delegate->base.write = filesystemDelegateWrite;
	delegate->base.getLength = filesystemDelegateGetLength;
	delegate->base.refreshDir = filesystemDelegateRefresh;
	delegate->base.create = filesystemDelegateCreate;
	delegate->base.truncate = filesystemDelegateTruncate;
	delegate->base.rename = filesystemDelegateRename;
	delegate->base.makeDirectory = filesystemDelegateMakeDirectory;
	delegate->base.mutate = filesystemDelegateMutate;
}

static g_fs_task_delegate* filesystemDelegateAllocate(g_task* task, const char* name)
{
	auto delegate = static_cast<g_fs_task_delegate*>(heapTryAllocate(sizeof(g_fs_task_delegate)));
	if(!delegate)
		return nullptr;

	memorySetBytes(delegate, 0, sizeof(*delegate));
	delegate->root = filesystemCreateDetachedNode(G_FS_NODE_TYPE_MOUNTPOINT, name);
	delegate->mountEntry = static_cast<g_fs_node_entry*>(heapTryAllocate(sizeof(g_fs_node_entry)));
	if(!delegate->root || !delegate->mountEntry || !filesystemDelegateMapPages(task, delegate))
	{
		if(delegate->root)
			filesystemDiscardDetachedNode(delegate->root);
		if(delegate->mountEntry)
			heapFree(delegate->mountEntry);
		heapFree(delegate);
		return nullptr;
	}

	return delegate;
}

g_fs_delegate_status filesystemDelegateRegister(g_task* task, g_syscall_fs_delegate_register* data)
{
	if(data->version != G_FS_DELEGATE_VERSION || data->storage_size != sizeof(g_fs_delegate_transaction_storage))
		return G_FS_DELEGATE_UNSUPPORTED;

	g_mutex_guard guard(&filesystemDelegateLock);
	if(filesystemFindExistingChild(filesystemGetMountFolder(), data->name))
		return G_FS_DELEGATE_ALREADY_EXISTS;
	if(filesystemDelegateActiveCount >= G_FS_DELEGATE_MAX_ACTIVE ||
	   filesystemDelegateRecordCount >= G_FS_DELEGATE_MAX_RECORDS ||
	   filesystemDelegateNodeCount >= G_FS_DELEGATE_MAX_NODES)
		return G_FS_DELEGATE_BUSY;

	auto delegate = filesystemDelegateAllocate(task, data->name);
	if(!delegate)
		return G_FS_DELEGATE_NO_MEMORY;

	filesystemDelegateInitializeCallbacks(delegate);
	delegate->owner = task->id;
	delegate->root->physicalId = data->phys_mountpoint_id;
	delegate->root->delegate = &delegate->base;
	delegate->root->parent = filesystemGetMountFolder();
	delegate->online = true;
	delegate->next = filesystemDelegates;
	filesystemDelegates = delegate;
	++filesystemDelegateRecordCount;
	++filesystemDelegateActiveCount;
	++filesystemDelegateNodeCount;
	filesystemPublishNode(delegate->root, delegate->mountEntry);
	data->mountpoint_id = delegate->root->id;
	data->transaction_storage = (g_fs_delegate_transaction_storage*) delegate->mapping;
	return G_FS_DELEGATE_SUCCESSFUL;
}

/**
 * Promotes a ready filesystem service to the boot root. Its delegate identity
 * and existing descriptors survive; unregistering it later marks it offline.
 */
bool filesystemDelegateMountRoot(const char* name)
{
	g_mutex_guard guard(&filesystemDelegateLock);
	for(auto delegate = filesystemDelegates; delegate; delegate = delegate->next)
	{
		if(!delegate->online || !delegate->mountEntry || !stringEquals(delegate->root->name, name))
			continue;

		filesystemDetachNode(delegate->root, delegate->mountEntry);
		delegate->mountEntry = nullptr;
		filesystemAdoptRoot(delegate->root);
		return true;
	}

	return false;
}

g_fs_delegate_status filesystemDelegateUnregister(g_task* task, g_fs_virt_id mount, bool force)
{
	g_mutex_guard guard(&filesystemDelegateLock);
	auto delegate = filesystemDelegateFindMount(mount);
	if(!delegate || !delegate->online)
		return G_FS_DELEGATE_NOT_FOUND;
	if(delegate->owner != task->id)
		return G_FS_DELEGATE_NOT_OWNER;
	if(delegate->transaction && !force)
		return G_FS_DELEGATE_BUSY;

	filesystemDelegateDetach(delegate);
	filesystemDelegateReleasePages(task, delegate);
	return G_FS_DELEGATE_SUCCESSFUL;
}

void filesystemDelegateTaskRemoved(g_task* task)
{
	g_mutex_guard guard(&filesystemDelegateLock);
	for(auto delegate = filesystemDelegates; delegate; delegate = delegate->next)
	{
		if(delegate->owner != task->id)
			continue;

		filesystemDelegateDetach(delegate);
		filesystemDelegateReleasePages(task, delegate);
	}
}
