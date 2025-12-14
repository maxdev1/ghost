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

#include "libterminal/terminal.hpp"
#include <ghost.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <unistd.h>

/**
 * Use to buffer stray input whne responses to terminal requests are read.
 */
static int* buffer = nullptr;
static int bufferedChars = 0;
static int bufferSize = 64;
static g_user_mutex bufferLock = g_mutex_initialize();

static void terminalCommand(const char* fmt, ...)
{
	fputc((char) G_TERMKEY_ESC, stdout);
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fflush(stdout);
}

/**
 *
 */
void g_terminal::setEcho(bool echo)
{
	terminalCommand("{%de", echo ? 1 : 0);
}

/**
 *
 */
void g_terminal::setMode(g_terminal_mode mode)
{
	terminalCommand("{%dm", (int) mode);
}

/**
 *
 */
int g_terminal::readUnbuffered()
{
	int c = getc(stdin);

	// Escaped sequences
	if(c == G_TERMKEY_SUB)
	{
		int b1 = getc(stdin);
		if(b1 == -1)
		{
			return -1;
		}
		int b2 = getc(stdin);
		if(b2 == -1)
		{
			return -1;
		}
		return ((b2 << 8) | b1);
	}

	return c;
}

/**
 *
 */
void g_terminal::bufferChar(int c)
{
	g_mutex_acquire(bufferLock);

	// Create buffer
	if(buffer == 0)
	{
		buffer = new int[bufferSize];
		if(!buffer)
		{
			klog("failed to buffer char in terminal client, could not allocate buffer");
			return;
		}

		bufferedChars = 0;
	}

	// Put char in buffer
	if(bufferedChars < bufferSize)
	{
		buffer[bufferedChars++] = c;
	}

	g_mutex_release(bufferLock);
}

/**
 *
 */
void g_terminal::readAndBufferUntilESC()
{

	int c;
	while((c = readUnbuffered()) != G_TERMKEY_ESC)
	{
		bufferChar(c);
	}
	// now ESC was read
}

/**
 *
 */
int g_terminal::getChar()
{

	int c;
	g_mutex_acquire(bufferLock);

	// If there are chars in the buffer, take these
	if(buffer && bufferedChars > 0)
	{
		c = buffer[0];
		memcpy(buffer, &buffer[1], (bufferSize - 1) * sizeof(int));
		--bufferedChars;
	}
	else
	{
		c = readUnbuffered();
	}

	g_mutex_release(bufferLock);
	return c;
}

/**
 *
 */
void g_terminal::putChar(int c)
{
	terminalCommand("{%dp", c);
}

/**
 *
 */
void g_terminal::setCursor(g_term_cursor_position position)
{
	terminalCommand("[%d;%df", position.y, position.x);
}

/**
 *
 */
int g_terminal::readEscapedParameters(int* parameters)
{

	int parameterCount = 0;

	int c;
	while(true)
	{
		c = readUnbuffered();

		if(c >= '0' && c <= '9')
		{
			if(parameterCount == 0)
			{
				parameterCount = 1;
			}
			if(parameterCount <= TERMINAL_STREAM_CONTROL_MAX_PARAMETERS)
			{
				parameters[parameterCount - 1] = parameters[parameterCount - 1] * 10;
				parameters[parameterCount - 1] += c - '0';
			}
		}
		else if(c == ';')
		{
			parameterCount++;
		}
		else
		{
			break;
		}
	}

	return c;
}

/**
 *
 */
g_term_cursor_position g_terminal::getCursor()
{

	// read request
	terminalCommand("[6n");

	// read response
	readAndBufferUntilESC();

	int ch = readUnbuffered();
	if(ch == '[')
	{
		int parameters[TERMINAL_STREAM_CONTROL_MAX_PARAMETERS];
		for(int i = 0; i < TERMINAL_STREAM_CONTROL_MAX_PARAMETERS; i++)
		{
			parameters[i] = 0;
		}

		int mode = readEscapedParameters(parameters);

		if(mode == 'R')
		{
			g_term_cursor_position result;
			result.y = parameters[0];
			result.x = parameters[1];
			return result;
		}
	}

	g_term_cursor_position invalid;
	invalid.y = -1;
	invalid.x = -1;
	return invalid;
}

/**
 *
 */
g_term_dimension g_terminal::getSize()
{

	// read request
	terminalCommand("{i");

	// read response
	readAndBufferUntilESC();

	int ch = readUnbuffered();
	if(ch == '{')
	{
		int parameters[TERMINAL_STREAM_CONTROL_MAX_PARAMETERS];
		for(int i = 0; i < TERMINAL_STREAM_CONTROL_MAX_PARAMETERS; i++)
		{
			parameters[i] = 0;
		}

		int mode = readEscapedParameters(parameters);

		if(mode == 'i')
		{
			g_term_dimension result;
			result.w = parameters[0];
			result.h = parameters[1];
			return result;
		}
	}

	g_term_dimension invalid;
	invalid.w = -1;
	invalid.h = -1;
	return invalid;
}

/**
 *
 */
void g_terminal::moveCursorUp(int n)
{
	terminalCommand("[%dA", n);
}

/**
 *
 */
void g_terminal::moveCursorDown(int n)
{
	terminalCommand("[%dB", n);
}

/**
 *
 */
void g_terminal::moveCursorForward(int n)
{
	terminalCommand("[%dC", n);
}

/**
 *
 */
void g_terminal::moveCursorBack(int n)
{
	terminalCommand("[%dD", n);
}

/**
 *
 */
void g_terminal::remove()
{
	terminalCommand("{x");
}

/**
 *
 */
void g_terminal::setControlProcess(g_pid pid)
{
	terminalCommand("{%dc", pid);
}

/**
 *
 */
void g_terminal::clear()
{
	terminalCommand("[2J");
}

/**
 *
 */
void g_terminal::setScrollAreaToScreen()
{
	terminalCommand("[r");
}

/**
 *
 */
void g_terminal::setScrollArea(int start, int end)
{
	terminalCommand("[%d;%dr", start, end);
}

/**
 *
 */
void g_terminal::scroll(int value)
{
	if(value >= 0)
	{
		terminalCommand("[%dS", value);
	}
	else
	{
		terminalCommand("[%dT", -value);
	}
}

/**
 *
 */
void g_terminal::setCursorVisible(bool visible)
{
	terminalCommand("{0;%dC", visible ? 1 : 0);
}
