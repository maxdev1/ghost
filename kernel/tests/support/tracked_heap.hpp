#pragma once

#include <algorithm>
#include <type_traits>
#include <unordered_set>
#include <mutex>
std::unordered_set<void*> heapBlocks;
std::mutex heapBlocksLock;
void* heapAllocate(size_t size) {
    auto p = std::malloc(size ? size : 1);
    EXPECT_TRUE(p);
    std::memset(p, 0xa5, size);
    std::lock_guard<std::mutex> guard(heapBlocksLock);
    EXPECT_TRUE(heapBlocks.insert(p).second);
    return p;
}
void* heapAllocateClear(size_t size) {
    auto p = heapAllocate(size);
    std::memset(p, 0, size);
    return p;
}
void heapFree(void* p) {
    if(!p) return;
    std::lock_guard<std::mutex> guard(heapBlocksLock);
    EXPECT_EQ(1, heapBlocks.erase(p));
    std::free(p);
}
char* stringDuplicate(const char* s) {
    auto p = static_cast<char*>(heapAllocate(std::strlen(s) + 1));
    std::strcpy(p, s);
    return p;
}
int stringHash(const char* s) { return int(std::strlen(s)); }
bool stringEquals(const char* a, const char* b) { return std::strcmp(a, b) == 0; }
