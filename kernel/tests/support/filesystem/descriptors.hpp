#pragma once
#include "support/tracked_heap.hpp"

#include "support/synchronization.hpp"
#include "production/hashmap_hpp.inc"

using g_fd = int; using g_pid = int; using g_tid = int;
using g_fs_virt_id = int; using g_file_flag_mode = int;
using g_fs_open_status = int; using g_fs_close_status = int;
using g_fs_clonefd_status = int; using g_fs_read_status = int; using g_fs_write_status = int;
constexpr int G_FD_NONE = -1, G_PID_NONE = -1;
constexpr int G_FS_OPEN_SUCCESSFUL = 0, G_FS_OPEN_ERROR = 1;
constexpr int G_FS_CLOSE_SUCCESSFUL = 0, G_FS_CLOSE_INVALID_FD = 1;
constexpr int G_FS_READ_SUCCESSFUL = 0, G_FS_READ_BUSY = 1, G_FS_READ_ERROR = 2, G_FS_READ_INVALID_FD = 3;
constexpr int G_FS_WRITE_SUCCESSFUL = 0, G_FS_WRITE_BUSY = 1, G_FS_WRITE_ERROR = 2, G_FS_WRITE_INVALID_FD = 3;
constexpr int G_FILE_FLAG_MODE_BLOCKING = 1, G_FILE_FLAG_MODE_APPEND = 2, G_FS_LENGTH_SUCCESSFUL = 0;
struct g_wait_queue { uint64_t bits; };
struct g_process { int id; } process{42};
struct g_task { g_process* process; int id; std::atomic<bool> waiting; } task{&process, 1, {false}};
void waitQueueInitialize(g_wait_queue* q) { q->bits = 0; }
void waitQueueAdd(g_wait_queue* q, int id) { q->bits |= 1ULL << id; }
void waitQueueRemove(g_wait_queue* q, int id) { q->bits &= ~(1ULL << id); }
void waitQueueRemoveTask(int) {}
void taskingWake(g_task* t) { t->waiting = false; }
void waitQueueWake(g_wait_queue* q) { if(q->bits) taskingWake(&task); q->bits = 0; }
struct g_fs_node { int id; } nodes[8] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}};
struct g_fs_delegate {
    int (*open)(g_fs_node*, int);
    int (*close)(g_fs_node*, int);
    bool (*waitForRead)(g_task*, g_fs_node*, uint64_t);
    bool (*waitForWrite)(g_task*, g_fs_node*, uint64_t);
};
std::atomic<int> closedNodes{0};
int openNode(g_fs_node*, int) { return 0; }
int closeNode(g_fs_node*, int) { ++closedNodes; return 0; }
bool waitNode(g_task* t, g_fs_node*, uint64_t) { t->waiting = true; return true; }
g_fs_delegate delegate{openNode, closeNode, waitNode, waitNode};
g_fs_delegate* filesystemFindDelegate(g_fs_node*) { return &delegate; }
g_fs_node* filesystemGetNode(int id) { return &nodes[id]; }
std::mutex phaseLock;
std::condition_variable phaseChanged;
bool waitingPrepared, proceed, cancel, inRead, allowRead;
int mode;
struct Cancelled {};
void taskingHandleTerminationRequest() { if(cancel) throw Cancelled{}; }
template<class P, class A> bool taskingWaitIf(P prepare, A after) {
    if(!prepare()) return false;
    std::unique_lock<std::mutex> lock(phaseLock);
    waitingPrepared = true; phaseChanged.notify_all();
    phaseChanged.wait(lock, [] { return proceed; });
    lock.unlock();
    after();
    taskingHandleTerminationRequest();
    return true;
}
int filesystemRead(g_fs_node*, uint8_t*, uint64_t, uint64_t, int64_t* out) {
    if(mode == 2) {
        std::unique_lock<std::mutex> lock(phaseLock);
        inRead = true; phaseChanged.notify_all();
        phaseChanged.wait(lock, [] { return allowRead; });
    }
    *out = mode == 1 ? 0 : 1;
    return mode == 1 ? G_FS_READ_BUSY : G_FS_READ_SUCCESSFUL;
}
int writeStatus = G_FS_WRITE_BUSY, lengthStatus = G_FS_LENGTH_SUCCESSFUL;
int writeCalls = 0;
int64_t writeCount = 0;
uint64_t writeOffset = 0;
int filesystemWrite(g_fs_node*, uint8_t*, uint64_t offset, uint64_t, int64_t* out) {
    ++writeCalls; writeOffset = offset;
    *out = writeCount; return writeStatus;
}
int filesystemGetLength(g_fs_node*, uint64_t* out) { *out = 4; return lengthStatus; }

#include "production/filesystem_process_hpp.inc"
#include "production/filesystem_process_cpp.inc"
#include "production/filesystem_task_io.inc"
