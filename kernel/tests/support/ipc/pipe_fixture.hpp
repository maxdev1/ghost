#pragma once
#include "support/tracked_heap.hpp"
#include "production/pipe_atomic_capacity.inc"
#include "production/pipe_stream_capacity.inc"
#include "support/ipc/pipe_types.hpp"
#include "production/pipe_layout.inc"

g_pipeline* published;
int map, *pipeMap = &map, nextId;
template<typename K, typename V> void hashmapPut(int*, K, V value) { published = value; }
int pipeGetNextId() { return ++nextId; }
g_pipeline* pipeGetById(int id) { return id == nextId ? published : nullptr; }
void destroy() { heapFree(published->buffer); heapFree(published); published = nullptr; EXPECT_TRUE(heapBlocks.empty()); }
#include "production/pipeCanWrite.inc"
#include "production/pipeCreate.inc"
#include "production/pipeRead.inc"
#include "production/pipeWrite.inc"
#include "production/pipeWaitForRead.inc"
#include "production/pipeWaitForWrite.inc"

