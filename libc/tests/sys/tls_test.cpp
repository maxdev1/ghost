#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace
{
struct TlsIndex
{
	uintptr_t module;
	uintptr_t offset;
};

struct ThreadStorage
{
	uintptr_t self;
	int error;
	int other;
};

thread_local ThreadStorage storage;
}

extern "C" void* g_task_get_tls()
{
	return &storage.self;
}

extern "C" void* ghost___tls_get_addr(TlsIndex* index);
extern "C" void* ghost__tls_get_addr(TlsIndex* index);

TEST(tls, library_error_does_not_overwrite_thread_pointer)
{
	storage = {reinterpret_cast<uintptr_t>(&storage.self), 0, 123};
	TlsIndex error{1, offsetof(ThreadStorage, error)};
	*static_cast<int*>(ghost___tls_get_addr(&error)) = 8; // EBADF in Ghost

	EXPECT_EQ(reinterpret_cast<uintptr_t>(&storage.self), storage.self);
	EXPECT_EQ(8, storage.error);
	EXPECT_EQ(123, storage.other);

	TlsIndex other{1, offsetof(ThreadStorage, other)};
	EXPECT_EQ(&storage.other, ghost__tls_get_addr(&other));
}

TEST(tls, library_offsets_use_the_full_pointer_width)
{
	const uintptr_t base = reinterpret_cast<uintptr_t>(g_task_get_tls());
	for(uintptr_t offset : {uintptr_t(0), uintptr_t(8), uintptr_t(-16), UINTPTR_MAX / 2})
	{
		TlsIndex index{1, offset};
		EXPECT_EQ(base + offset, reinterpret_cast<uintptr_t>(ghost___tls_get_addr(&index)));
		EXPECT_EQ(base + offset, reinterpret_cast<uintptr_t>(ghost__tls_get_addr(&index)));
	}
}
