#ifdef _GHOST_

#include "platform/platform.hpp"
#include "platform/ghost/ghost.hpp"
#include "windowserver.hpp"

#include <libinput/keyboard/keyboard.hpp>
#include <libinput/mouse/mouse.hpp>
#include <components/cursor.hpp>

windowserver_t* server = nullptr;

int main()
{
	server = new windowserver_t();
	server->launch();
	return 0;
}

SYS_MUTEX_T platformInitializeMutex(bool reentrant)
{
	return g_mutex_initialize_r(reentrant);
}

void platformStartInput()
{
	inputReceiverInitialize();
}

SYS_TID_T platformGetPidForTid(SYS_TID_T tid)
{
	return g_get_pid_for_tid(tid);
}

void* platformAllocateMemory(size_t size)
{
	return g_alloc_mem(size);
}


void* platformShareMemory(void* memory, size_t size, SYS_TID_T target)
{
	return g_share_mem(memory, size, target);
}

void platformLog(const char* message, ...)
{
	va_list l;
	va_start(l, message);
	kvlog(message, l);
	va_end(l);
}

SYS_TID_T platformCreateThread(void* entry)
{
	return g_create_task(entry);
}

SYS_TID_T platformCreateThreadWithData(void* entry, void* data)
{
	return g_create_task_d(entry, data);
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

void platformAcquireMutexTimeout(SYS_MUTEX_T mutex, uint32_t timeout)
{
	g_mutex_acquire_to(mutex, timeout);
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

char platformCharForKey(g_key_info info)
{
	return g_keyboard::charForKey(info);
}

uint64_t platformMillis()
{
	return g_millis();
}

void platformJoin(SYS_TID_T tid)
{
	g_join(tid);
}

bool platformRegisterTaskIdentifier(const char* identifier)
{
	return g_task_register_name(identifier);
}

SYS_TID_T platformSpawn(const char* path, const char* args, const char* cwd)
{
	return g_spawn(path, args, cwd, G_SECURITY_LEVEL_APPLICATION);
}

bool platformInitializeKeyboardLayout(std::string layout)
{
	return g_keyboard::loadLayout(layout);
}

void platformSleep(uint64_t time)
{
	g_sleep(time);
}

SYS_TID_T platformGetTid()
{
	return g_get_tid();
}

void platformExit(int v)
{
	g_exit(v);
}

void platformLoadCursors()
{
	auto dir = g_open_directory("/system/graphics/cursor");
	g_fs_directory_entry* entry;
	while((entry = g_read_directory(dir)) != nullptr)
	{
		std::string path = std::string("/system/graphics/cursor") + "/" + entry->name;
		cursor_t::load(path);
	}
	cursor_t::set("default");
}

#endif
