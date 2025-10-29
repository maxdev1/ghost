#ifdef _GHOST_

#include "libwindow/platform/platform.hpp"
#include "libwindow/interface.hpp"

#include <cstdarg>
#include <ghost.h>

SYS_TID_T platformAwaitUiRegistry() {
	return g_task_await_by_name(G_UI_REGISTRY_NAME);
}

void platformLog(const char* message, ...) {
	va_list l;
	va_start(l, message);
	kvlog(message, l);
	va_end(l);
}

SYS_TID_T platformCreateThread(void* entry)
{
	return g_create_task(entry);
}

SYS_MUTEX_T platformInitializeMutex(bool reentrant)
{
	return g_mutex_initialize_r(true);
}

SYS_TX_T platformCreateMessageTransaction()
{
	return g_get_message_tx_id();
}

bool platformSendMessage(SYS_TID_T tid, void* buf, size_t len, SYS_TX_T tx)
{
	return g_send_message_t(tid, buf, len, tx) == G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

int platformReceiveMessage(void* buf, size_t max, SYS_TX_T tx)
{
	return g_receive_message_t(buf, max, tx);
}

void platformAcquireMutex(SYS_MUTEX_T mutex)
{
	g_mutex_acquire(mutex);
}

void platformReleaseMutex(SYS_MUTEX_T mutex)
{
	g_mutex_release(mutex);
}

void platformYieldTo(SYS_TID_T tid)
{
	g_yield_t(tid);
}

void platformUnmapSharedMemory(void* mem)
{
	g_unmap(mem);
}

#endif
