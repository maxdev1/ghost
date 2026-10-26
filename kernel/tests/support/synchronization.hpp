#pragma once

#include <map>
#include <memory>
#include <mutex>
std::mutex lockIndexGuard;
std::map<g_mutex*, std::shared_ptr<std::recursive_mutex>> lockIndex;
std::shared_ptr<std::recursive_mutex> lockFor(g_mutex* m) {
    std::lock_guard<std::mutex> guard(lockIndexGuard);
    auto& lock = lockIndex[m];
    if(!lock) lock = std::make_shared<std::recursive_mutex>();
    return lock;
}
void realAcquire(g_mutex* m) { lockFor(m)->lock(); ++m->depth; }
void realRelease(g_mutex* m) { EXPECT_TRUE(m->depth > 0); --m->depth; lockFor(m)->unlock(); }
#define mutexAcquire realAcquire
#define mutexRelease realRelease
void mutexInitializeTask(g_mutex* m, const char* = nullptr) { m->depth = 0; }
