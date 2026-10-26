#pragma once
using g_security_level = uint8_t;
using g_pid = int; using g_tid = int; using g_fd = int;
using g_create_task_status = int; using g_spawn_status = int;
using g_spawn_validation_details = int; using g_fs_open_status = int;
constexpr int G_TID_NONE = -1, G_TASK_CORE_AFFINITY_NONE = 255;
constexpr int G_SECURITY_LEVEL_KERNEL = 0, G_SECURITY_LEVEL_DRIVER = 1, G_SECURITY_LEVEL_APPLICATION = 2;
constexpr int G_CREATE_TASK_STATUS_FAILED = 1, G_CREATE_TASK_STATUS_SUCCESSFUL = 0;
constexpr int G_SPAWN_STATUS_SUCCESSFUL = 0, G_SPAWN_STATUS_TASKING_ERROR = 1, G_SPAWN_STATUS_IO_ERROR = 2;
constexpr int G_FS_OPEN_SUCCESSFUL = 0, G_FILE_FLAG_MODE_READ = 1;
#include "production/create_task_callstruct.inc"
#include "production/spawn_callstruct.inc"

struct g_task;
struct g_process { g_task* main; int id; struct { const char* executablePath; const char* arguments; char* workingDirectory; } environment; };
struct g_task { g_process* process; int securityLevel, id; struct { void* function; void* data; } userEntry; } child{};
g_syscall_create_task* mutateCreate; g_syscall_spawn* mutateSpawn;
int created, assignedCore = -1, opened, spawnedLevel = -1;
char pathName[] = "/test";
int processorGetNumberOfProcessors() { return 2; }
g_task* taskingCreateTask(uintptr_t, g_process*, int) {
    ++created; child.id = 42;
    if(mutateCreate) { mutateCreate->coreAffinity = 255; mutateCreate->userEntry = nullptr; mutateCreate->userData = nullptr; }
    return &child;
}
void taskingAssignBalanced(g_task*) { assignedCore = 255; }
void taskingAssignOnCore(uint8_t core, g_task*) { assignedCore = core; }
int filesystemOpen(const char*, int, g_task*, int* fd) {
    *fd = 7; ++opened; if(mutateSpawn) mutateSpawn->securityLevel = 0; return 0;
}
struct g_spawn_result { int status, validation; g_process* process; };
g_spawn_result taskingSpawn(int, uint8_t level) { spawnedLevel = level; return {2, 0, nullptr}; }
void filesystemClose(int, int, bool) {}
void filesystemProcessCreateStdio(int, const int*, int, int*) {}
char* stringDuplicate(const char*) { std::abort(); }
void taskingReleaseSpawn(g_process*) {}

#include "production/syscallCreateTask_and_spawn.inc"

