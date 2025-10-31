#ifdef _WIN32

#include "libwindow/platform/platform.hpp"
#include "libwindow/interface.hpp"
#include <cstdarg>
#include <stdio.h>

SYS_TID_T platformAwaitUiRegistry()
{
	return NULL;
}

void platformLog(const char* message, ...)
{
	va_list l;
	va_start(l, message);
	vprintf(message, l);
	va_end(l);
	printf("\n");
}

SYS_MUTEX_T platformInitializeMutex(bool reentrant)
{
	win_mutex_t* mutex = (win_mutex_t*) malloc(sizeof(win_mutex_t));
	mutex->reentrant = reentrant;
	if (reentrant)
		InitializeCriticalSection(&mutex->cs);
	else
		mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
	return mutex;
}

void platformAcquireMutex(SYS_MUTEX_T m)
{
	if (m->reentrant) {
		EnterCriticalSection(&m->cs);
	} else {
		WaitForSingleObject(m->handle, INFINITE);
	}
}

void platformReleaseMutex(SYS_MUTEX_T m)
{
	if (m->reentrant) {
		LeaveCriticalSection(&m->cs);
	} else {
		ReleaseSemaphore(m->handle, 1, NULL);
	}
}


SYS_TX_T platformCreateMessageTransaction()
{
}

bool platformSendMessage(SYS_TID_T tid, void* buf, size_t len, SYS_TX_T tx)
{
};

int platformReceiveMessage(void* buf, size_t max, SYS_TX_T tx)
{
}

void platformYieldTo(SYS_TID_T tid)
{
}

void platformUnmapSharedMemory(void* mem)
{
}

DWORD WINAPI _platformThreadEntry(LPVOID arg)
{
	void (*entry)() = (void (*)()) arg;
	entry();
	return 0;
}

SYS_TID_T platformCreateThread(void* entry)
{
	return CreateThread(NULL, 0, _platformThreadEntry, entry, 0, NULL);
}

#endif
