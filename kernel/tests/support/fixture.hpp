#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>


using g_address = uintptr_t;
using g_virtual_address = uintptr_t;
using g_physical_address = uintptr_t;
using g_size = uintptr_t;
typedef int g_spinlock;
#include "production/mutex_layout.inc"
inline void mutexInitializeGlobal(g_mutex* m, const char* = nullptr) { m->depth = 0; }
inline void mutexAcquire(g_mutex* m) { ++m->depth; }
inline void mutexRelease(g_mutex* m) { EXPECT_TRUE(m->depth > 0); --m->depth; }
#define logInfo(...) ((void)0)
#define logWarn(...) ((void)0)
#define logDebug(...) ((void)0)
#define panic(...) std::abort()
