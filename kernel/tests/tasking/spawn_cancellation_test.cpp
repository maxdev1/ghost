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
#include "production/hashmap_hpp.inc"

using g_tid = int;
constexpr int G_TID_NONE = -1;
struct g_task { int id, spawnParent; bool spawnPending, spawnReleased = false; };
g_hashmap<int, g_task*>* taskGlobalMap;
g_task* taskingAcquireTask(int id) { return hashmapGet(taskGlobalMap, id, static_cast<g_task*>(nullptr)); }
void taskingReleaseTask(g_task*) {}
std::vector<int> requested;
void taskingRequestTermination(g_task* task) { requested.push_back(task->id); }

#include "production/taskingCancelPendingSpawns.inc"

TEST(SpawnCancellation, TargetsOnlyPendingChildrenOfTheExitingParent) {
    taskGlobalMap = hashmapCreateNumeric<int, g_task*>(16);
    g_task children[] = {{9, 42, true}, {8, 42, true}, {7, 43, true}, {3, 42, false}, {1, 42, true},
        {5, 42, true, true}};
    for(auto& child : children) hashmapPut(taskGlobalMap, child.id, &child);
    taskingCancelPendingSpawns(42);
    EXPECT_TRUE((requested == std::vector<int>{1, 8, 9}));
    requested.clear();
    taskingCancelPendingSpawns(99);
    EXPECT_TRUE(requested.empty());
    hashmapDestroy(taskGlobalMap);
    EXPECT_TRUE(heapBlocks.empty());
}

TEST(SpawnCancellation, PreservesReleasedChildrenBeforeTheyFinishTheirHandshake)
{
    taskGlobalMap = hashmapCreateNumeric<int, g_task*>(16);
    g_task child{17, 42, true, true};
    hashmapPut(taskGlobalMap, child.id, &child);
    requested.clear();
    taskingCancelPendingSpawns(42);

    EXPECT_TRUE(requested.empty());
    EXPECT_TRUE(child.spawnPending);

    hashmapDestroy(taskGlobalMap);

    EXPECT_TRUE(heapBlocks.empty());
}
}
