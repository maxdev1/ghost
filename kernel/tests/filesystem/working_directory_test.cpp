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
#include "support/tracked_heap.hpp"

#include "support/synchronization.hpp"

using g_address = uintptr_t;
constexpr int G_PATH_MAX = 4096;
void* heapTryAllocate(size_t size) { return heapAllocate(size); }
constexpr int G_GET_WORKING_DIRECTORY_SUCCESSFUL = 0, G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED = 1;
constexpr int G_SET_WORKING_DIRECTORY_SUCCESSFUL = 0, G_SET_WORKING_DIRECTORY_NOT_A_FOLDER = 1;
constexpr int G_SET_WORKING_DIRECTORY_NOT_FOUND = 2, G_SET_WORKING_DIRECTORY_ERROR = 3;
constexpr int G_FS_OPEN_SUCCESSFUL = 0, G_FS_OPEN_NOT_FOUND = 1;
constexpr int G_FS_NODE_TYPE_FOLDER = 0, G_FS_NODE_TYPE_MOUNTPOINT = 1, G_FS_NODE_TYPE_ROOT = 2;
struct g_process { g_mutex lock; struct { char* workingDirectory; } environment; } process{};
struct g_task { g_process* process; } task{&process};
struct g_syscall_get_working_directory { char* buffer; size_t maxlen; int result; };
struct g_syscall_set_working_directory { const char* path; int result; };
struct node { int type; } directory{0};
struct find_result { int status; struct node* node; };
find_result filesystemFind(void*, const char*) { return {0, &directory}; }
int filesystemGetAbsolutePathLength(node*) { return 8; }
int filesystemGetAbsolutePath(node*, char* out, size_t) { std::strcpy(out, "/updated"); return 8; }
size_t stringLength(const char* p) { return std::strlen(p); }
void stringCopy(char* out, const char* p) { std::strcpy(out, p); }

bool memoryCopyUserString(g_task*, const char* source, char* target, size_t capacity) {
    const char* path = source ? source : "/updated";
    if(strlen(path) >= capacity) return false;
    strcpy(target, path); return true;
}
bool memoryCopyToUser(g_task*, g_address address, const void* source, size_t count) {
    memcpy((void*) address, source, count); return true;
}
#include "production/syscall_working_directory.inc"

TEST(WorkingDirectory, ReturnsBoundedSnapshotsDuringConcurrentDirectoryChanges) {
    char buf[64]; g_syscall_get_working_directory request{buf, 0, 99};
    syscallGetWorkingDirectory(&task, &request); EXPECT_EQ(1, request.result);
    request.maxlen = 1; syscallGetWorkingDirectory(&task, &request); EXPECT_EQ(1, request.result);
    request.maxlen = 2; syscallGetWorkingDirectory(&task, &request); EXPECT_TRUE(request.result == 0 && !std::strcmp(buf, "/"));
    std::thread writer([] {
        for(int i = 0; i < 2000; ++i) {
            g_syscall_set_working_directory set{}; syscallSetWorkingDirectory(&task, &set); EXPECT_EQ(0, set.result);
        }
    });
    std::thread reader([] {
        for(int i = 0; i < 2000; ++i) {
            char value[64]; g_syscall_get_working_directory get{value, sizeof(value), 99};
            syscallGetWorkingDirectory(&task, &get);
            EXPECT_TRUE(get.result == 0 && (!std::strcmp(value, "/") || !std::strcmp(value, "/updated")));
        }
    });
    writer.join(); reader.join(); heapFree(process.environment.workingDirectory);
    EXPECT_TRUE(heapBlocks.empty());
}
}
