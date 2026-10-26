#pragma once

using g_fs_phys_id = int; using g_bool = bool; using g_wait_queue = int;
using g_fs_pipe_status = int; using g_fs_read_status = int; using g_fs_write_status = int;
constexpr int G_FS_PIPE_SUCCESSFUL = 0, G_FS_PIPE_ERROR = 1;
constexpr int G_FS_READ_SUCCESSFUL = 0, G_FS_READ_BUSY = 1, G_FS_READ_ERROR = 2;
constexpr int G_FS_WRITE_SUCCESSFUL = 0, G_FS_WRITE_BUSY = 1, G_FS_WRITE_ERROR = 2;
constexpr int G_PAGE_SIZE = 4096;
#define G_PAGE_ALIGN_UP(n) (((n) + 4095) & ~4095ULL)
void mutexInitializeTask(g_mutex* lock, const char*) { mutexInitializeGlobal(lock); }
void waitQueueInitialize(int* queue) { *queue = 0; }
int wakeups;
void waitQueueWake(int*) { ++wakeups; }
struct g_task { int id; bool waiting; };
void taskingPrepareWait(g_task* task, const char*) { task->waiting = true; }
void waitQueueAdd(int* queue, int) { ++*queue; }
void memoryCopy(void* target, const void* source, size_t size) { std::memcpy(target, source, size); }
bool failAllocation;
size_t allocatedPages;
uintptr_t memoryAllocateKernel(size_t pages) {
    allocatedPages = pages;
    return failAllocation ? 0 : reinterpret_cast<uintptr_t>(heapAllocateClear(pages * 4096));
}
