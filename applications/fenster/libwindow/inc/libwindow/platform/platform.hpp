#ifndef __LIBWINDOW_PLATFORM__
#define __LIBWINDOW_PLATFORM__

/**
 * Platform specific type definitions
 */
#include <stdint.h>

#include "platform-key-info.hpp"

/**
 * Ghost-specific type definitions
 */
#ifdef _GHOST_

#include <ghost.h>

#define SYS_TID_T g_tid
#define SYS_TID_NONE G_TID_NONE

#define SYS_MUTEX_T g_user_mutex

#define SYS_TX_T g_message_transaction
#define SYS_TX_NONE G_MESSAGE_TRANSACTION_NONE

#define SYS_MESSAGE_HEADER_SIZE sizeof(g_message_header)
#define SYS_MESSAGE_CONTENT(message) G_MESSAGE_CONTENT(message)
#define SYS_MESSAGE_RECEIVE_SUCCESS         G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL
#define SYS_MESSAGE_RECEIVE_ERROR_EXCEEDS_BUFFER  G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE


/**
 * Windows-MinGW-specific type definitions
 */
#elif _WIN32

#include <windows.h>

#define SYS_TID_T     HANDLE
#define SYS_TID_NONE  NULL

#ifndef __SYS_MUTEX_T_DEFINED__
#define __SYS_MUTEX_T_DEFINED__
typedef struct {
 HANDLE handle;
 CRITICAL_SECTION cs;
 bool reentrant;
} win_mutex_t;
#define SYS_MUTEX_T win_mutex_t*
#endif

#define SYS_TX_T      int
#define SYS_TX_NONE   -1

#define SYS_MESSAGE_HEADER_SIZE 0
#define SYS_MESSAGE_CONTENT(message) message
#define SYS_MESSAGE_RECEIVE_SUCCESS         true
#define SYS_MESSAGE_RECEIVE_ERROR_EXCEEDS_BUFFER  true

#include <string>

#endif


/**
 * Shall wait until the UI registry task is available to receive messages.
 */
SYS_TID_T platformAwaitUiRegistry();

/**
 * Initializes whatever is necessary to receive input.
 */
void platformLog(const char* message, ...);

SYS_MUTEX_T platformInitializeMutex(bool reentrant);

void platformAcquireMutex(SYS_MUTEX_T mutex);

void platformReleaseMutex(SYS_MUTEX_T mutex);

SYS_TX_T platformCreateMessageTransaction();

bool platformSendMessage(SYS_TID_T tid, void* buf, size_t len, SYS_TX_T tx);

int platformReceiveMessage(void* buf, size_t max, SYS_TX_T tx);

void platformYieldTo(SYS_TID_T tid);

void platformUnmapSharedMemory(void* mem);

SYS_TID_T platformCreateThread(void* entry);

#endif
