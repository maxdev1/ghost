/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "kernel/debug/debug_interface.hpp"
#include "kernel/debug/debug_interface_mode.hpp"
#include "kernel/system/serial_port.hpp"

bool debugInterfaceInitialized = false;
static uint16_t debugInterfaceComPort;

void debugInterfaceInitialize(uint16_t port)
{
	debugInterfaceInitialized = true;
	debugInterfaceComPort = port;
}

void debugInterfaceWriteByte(uint8_t value)
{
	serialPortWrite(debugInterfaceComPort, value);
}

void debugInterfaceWriteShort(uint16_t value)
{
	serialPortWrite(debugInterfaceComPort, (value >> 0) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 8) & 0xFF);
}

void debugInterfaceWriteInt(uint32_t value)
{
	serialPortWrite(debugInterfaceComPort, (value >> 0) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 8) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 16) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 24) & 0xFF);
}

void debugInterfaceWriteLong(uint64_t value)
{
	serialPortWrite(debugInterfaceComPort, (value >> 0) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 8) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 16) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 24) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 32) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 40) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 48) & 0xFF);
	serialPortWrite(debugInterfaceComPort, (value >> 56) & 0xFF);
}

void debugInterfaceWriteString(const char *string)
{
	const char *p = string;
	while (*p)
	{
		serialPortWrite(debugInterfaceComPort, *p++);
	}
	serialPortWrite(debugInterfaceComPort, 0);
}

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
static const int logBufferLength = 512;
static char logBuffer[logBufferLength];
static int logBuffered = 0;
#endif

void debugInterfaceWriteLogCharacter(char c)
{
	if (!debugInterfaceInitialized)
	{
		return;
	}

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_PLAIN_LOG
	serialPortWrite(debugInterfaceComPort, c);
#elif G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
	debugInterfaceFullWriteLogCharacter(c);
#endif
}

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
void debugInterfaceFullWriteLogCharacter(char c)
{
	logBuffer[logBuffered++] = c;

	if (c == '\n' || logBuffered == logBufferLength - 1)
	{
		debugInterfaceWriteShort(G_DEBUG_MESSAGE_LOG);
		for (int i = 0; i < logBuffered; i++)
		{
			serialPortWrite(debugInterfaceComPort, logBuffer[i]);
		}
		serialPortWrite(debugInterfaceComPort, 0);

		logBuffered = 0;
	}
}

void debugInterfacePublishTaskStatus(int tid, const char* status)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED;

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_TASK_SET_STATUS);
	debugInterfaceWriteInt(tid);
	debugInterfaceWriteString(status);
}

void debugInterfacePublishTaskRounds(int tid, long rounds)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED;

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_TASK_SET_ROUNDS);
	debugInterfaceWriteInt(tid);
	debugInterfaceWriteLong(rounds);
}

void debugInterfaceTaskSetIdentifier(int tid, const char* identifier)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_TASK_SET_IDENTIFIER);
	debugInterfaceWriteInt(tid);
	debugInterfaceWriteString(identifier);
}

void debugInterfaceTaskSetSourcePath(int tid, const char* source_path)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_TASK_SET_SOURCE_PATH);
	debugInterfaceWriteInt(tid);
	debugInterfaceWriteString(source_path);
}

void debugInterfaceFilesystemUpdateNode(int id, int physFsId, int parent, char* name)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_FILESYSTEM_UPDATE_NODE);
	debugInterfaceWriteInt(id);
	debugInterfaceWriteInt(physFsId);
	debugInterfaceWriteInt(parent);
	debugInterfaceWriteString(name);
}

void debugInterfaceMemorySetPageUsage(uint32_t pageaddr, int usage)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_MEMORY_SET_PAGE_USAGE);
	debugInterfaceWriteInt(pageaddr);
	debugInterfaceWriteByte(usage);
}

void debugInterfaceSystemInformation(const char* key, uint64_t value)
{
	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	debugInterfaceWriteShort(G_DEBUG_MESSAGE_SYSTEM_INFORMATION);
	debugInterfaceWriteString(key);
	debugInterfaceWriteLong(value);
}

#endif
