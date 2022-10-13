#ifndef __TEST__
#define __TEST__

#include <stdint.h>
#include <stdio.h>

void testAdd(const char* name, bool (*func)());

#define TEST(name, description)   \
	bool name();                  \
	class name##TestAdder         \
	{                             \
	  public:                     \
		name##TestAdder()         \
		{                         \
			testAdd(#name, name); \
		}                         \
	};                            \
	name##TestAdder name##Adder;  \
	bool name()

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define MOCK(name) TOKENPASTE2(name, __COUNTER__)

// Assert and panic calls
#define ASSERT_EQUALS(a, b) assertEquals<>(a, b, __LINE__)
#define ASSERT_NOT_EQUALS(a, b) assertNotEquals<>(a, b, __LINE__)

#define BUFLEN 512

template <typename T>
void assertEquals(T a, T b, int line)
{
	if(a != b)
	{
		char buf[BUFLEN];
		snprintf(buf, BUFLEN, "\t%i: assertion failed: expected %lli, got %lli\n", line, a, b);
		throw buf;
	}
}

template <typename T>
void assertEquals(T* a, T* b, int line)
{
	if(a != b)
	{
		char buf[BUFLEN];
		snprintf(buf, BUFLEN, "\t%i: assertion failed: expected %llx, got %llx\n", line, a, b);
		throw buf;
	}
}

template <typename T>
void assertNotEquals(T a, T b, int line)
{
	if(a == b)
	{
		char buf[BUFLEN];
		snprintf(buf, BUFLEN, "\t%i: assertion failed: expected not %lli, got %lli\n", line, a, b);
		throw buf;
	}
}

void _panic(int line, const char* msg, ...);

// Mock overrides
#define mutexInitialize(m)
#define _mutexInitialize(m)
#define mutexAcquire(m)
#define mutexRelease(m)

#define __GHOST_SYS_TYPES__
#define __GHOST_SYS_TYPES__
typedef uintptr_t g_address;
typedef g_address g_virtual_address;
typedef g_address g_physical_address;

#define __PANIC__
#define panic(msg...) _panic(__LINE__, msg);

#define __LOGGER__
#define logInfo(msg...)
#define logInfon(msg...)
#define logWarn(msg...)
#define logWarnn(msg...)
#define logDebug(msg...)
#define logDebugn(msg...)

#endif
