/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max SchlÃƒÂ¼ssel <lokoxe@gmail.com>                     *
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

#include <libterminal/terminal.hpp>
#include <unistd.h>

#include "screen/gui_screen.hpp"
#include "terminal.hpp"

static std::string defaultKeyboardLayout = "de-DE";

static g_pid shellProcess;
static g_fd shellStdin = G_FD_NONE;
static g_fd shellStdout = G_FD_NONE;
static g_fd shellStderr = G_FD_NONE;
static g_pid controlProcess = G_PID_NONE;

static screen_t* screen = nullptr;
static g_atom screenLock = g_atomic_initialize();

static g_terminal_mode inputMode = G_TERMINAL_MODE_DEFAULT;
static bool inputEcho = true;

int main(int argc, char* argv[])
{
	terminalInitializeScreen();
	if(!screen)
	{
		klog("terminal: failed to initialize screen");
		return -1;
	}
	screen->clean();

	if(!g_keyboard::loadLayout(defaultKeyboardLayout))
	{
		klog("terminal: failed to load keyboard layout: %s", defaultKeyboardLayout.c_str());
		return -1;
	}

	if(!terminalStartShell())
		return -1;

	output_routine_startinfo_t* outInfo = new output_routine_startinfo_t();
	outInfo->isStderr = false;
	g_create_thread_d((void*) &terminalOutputRoutine, outInfo);

	output_routine_startinfo_t* errInfo = new output_routine_startinfo_t();
	errInfo->isStderr = true;
	g_create_thread_d((void*) &terminalOutputRoutine, errInfo);

	terminalInputRoutine();
	return 0;
}

void terminalInitializeScreen()
{
	gui_screen_t* guiScreen = new gui_screen_t();
	if(guiScreen->initialize())
	{
		screen = guiScreen;
	}
	else
	{
		fprintf(stderr, "terminal: failed to initialize the graphical screen");
	}
}

bool terminalStartShell()
{
	g_fd shellStdinW;
	g_fd shellStdinR;
	if(g_pipe(&shellStdinW, &shellStdinR) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("terminal: failed to setup stdin pipe for shell");
		return false;
	}

	g_fd shellStdoutW;
	g_fd shellStdoutR;
	if(g_pipe(&shellStdoutW, &shellStdoutR) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("terminal: failed to setup stdout pipe for shell");
		return false;
	}

	g_fd shellStderrW;
	g_fd shellStderrR;
	if(g_pipe(&shellStderrW, &shellStderrR) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("terminal: failed to setup stderr pipe for shell");
		return false;
	}

	g_fd stdioIn[3];
	stdioIn[0] = shellStdinR;
	stdioIn[1] = shellStdoutW;
	stdioIn[2] = shellStderrW;

	auto shellStatus = g_spawn_poi("/applications/gsh.bin", "", "/", G_SECURITY_LEVEL_APPLICATION, &shellProcess, nullptr, stdioIn);
	if(shellStatus != G_SPAWN_STATUS_SUCCESSFUL)
	{
		klog("terminal: failed to spawn shell process");
		return false;
	}

	shellStdin = shellStdinW;
	shellStdout = shellStdoutR;
	shellStderr = shellStderrR;
	return true;
}

void terminalWriteToShell(std::string line)
{
	const char* lineContent = line.c_str();
	int lineLength = strlen(lineContent);

	int done = 0;
	int len = 0;
	while(done < lineLength)
	{
		len = write(shellStdin, &lineContent[done], lineLength - done);
		if(len <= 0)
		{
			break;
		}
		done += len;
	}
}

void terminalWriteTermkeyToShell(int termkey)
{
	char buf[3];
	buf[0] = G_TERMKEY_SUB;
	buf[1] = termkey & 0xFF;
	buf[2] = (termkey >> 8) & 0xFF;
	write(shellStdin, &buf, 3);
}

void terminalInputRoutine()
{
	std::string buffer = "";
	while(true)
	{
		g_key_info input = screen->readInput();

		// Default line-buffered input
		if(inputMode == G_TERMINAL_MODE_DEFAULT)
		{

			if(input.key == "KEY_ENTER" && input.pressed)
			{
				if(inputEcho)
				{
					g_atomic_lock(screenLock);
					screen->write('\n');
					g_atomic_unlock(screenLock);
				}

				buffer += '\n';
				terminalWriteToShell(buffer);

				buffer = "";
			}
			else if((input.ctrl && input.key == "KEY_C") || (input.key == "KEY_ESC"))
			{
				if(controlProcess && controlProcess != shellProcess)
				{
					g_kill(controlProcess);
				}
			}
			else if(input.key == "KEY_BACKSPACE" && input.pressed)
			{
				buffer = buffer.size() > 0 ? buffer.substr(0, buffer.size() - 1) : buffer;
				screen->backspace();
			}
			else
			{
				char chr = g_keyboard::charForKey(input);
				if(chr != -1)
				{
					buffer += chr;

					if(inputEcho)
					{
						g_atomic_lock(screenLock);
						screen->write(chr);
						g_atomic_unlock(screenLock);
					}
				}
			}
		}
		else if(inputMode == G_TERMINAL_MODE_RAW)
		{

			if(input.key == "KEY_ENTER" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_ENTER);

				if(inputEcho)
				{
					g_atomic_lock(screenLock);
					screen->write('\n');
					g_atomic_unlock(screenLock);
				}
			}
			else if(input.key == "KEY_BACKSPACE" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_BACKSPACE);

				if(inputEcho)
				{
					g_atomic_lock(screenLock);
					screen->backspace();
					g_atomic_unlock(screenLock);
				}
			}
			else if(input.key == "KEY_ARROW_LEFT" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_LEFT);
			}
			else if(input.key == "KEY_ARROW_RIGHT" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_RIGHT);
			}
			else if(input.key == "KEY_ARROW_UP" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_UP);
			}
			else if(input.key == "KEY_ARROW_DOWN" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_DOWN);
			}
			else if(input.key == "KEY_TAB" && input.pressed && input.shift)
			{
				terminalWriteToShell("\t");
			}
			else if(input.key == "KEY_TAB" && input.pressed)
			{
				terminalWriteTermkeyToShell(G_TERMKEY_STAB);
			}
			else
			{
				char chr = g_keyboard::charForKey(input);
				if(chr != -1)
				{
					write(shellStdin, &chr, 1);

					if(inputEcho)
					{
						g_atomic_lock(screenLock);
						screen->write(chr);
						g_atomic_unlock(screenLock);
					}
				}
			}
		}
		screen->flush();
	}
}

void terminalOutputRoutine(output_routine_startinfo_t* data)
{
	size_t bufSize = 1024;
	uint8_t* buf = new uint8_t[bufSize];

	stream_control_status_t status;

	while(true)
	{
		g_fs_read_status stat;
		int r = g_read_s(data->isStderr ? shellStderr : shellStdout, buf, bufSize, &stat);

		if(stat == G_FS_READ_SUCCESSFUL)
		{
			g_atomic_lock(screenLock);

			for(int i = 0; i < r; i++)
			{
				terminalProcessOutput(&status, data->isStderr, buf[i]);
			}

			screen->flush();
			g_atomic_unlock(screenLock);
		}
		else
		{
			break;
		}
	}

	delete buf;
	delete data;
}

void terminalProcessOutput(stream_control_status_t* status, bool isStderr, char c)
{
	// Simple textual output
	if(status->status == TERMINAL_STREAM_STATUS_TEXT)
	{
		if(c == '\r')
		{
			return;
		}
		else if(c == '\t')
		{
			screen->write(' ');
			screen->write(' ');
			screen->write(' ');
			screen->write(' ');
		}
		else if(c == 27 /* ESC */)
		{
			status->status = TERMINAL_STREAM_STATUS_LAST_WAS_ESC;
		}
		else
		{
			int fg = screen->getColorForeground();
			if(isStderr)
			{
				screen->setColorForeground(SC_RED);
			}
			screen->write(c);
			if(isStderr)
			{
				screen->setColorForeground(fg);
			}
		}
	}
	else if(status->status == TERMINAL_STREAM_STATUS_LAST_WAS_ESC) // Starting an escape sequence
	{

		if(c == '[') // must be followed by [ for VT100 sequence
		{
			status->status = TERMINAL_STREAM_STATUS_WITHIN_VT100;
		}
		else if(c == '{') // or a Ghost terminal sequence
		{
			status->status = TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM;
		}
		else // otherwise reset
		{
			status->status = TERMINAL_STREAM_STATUS_TEXT;
		}
	}
	else if(status->status == TERMINAL_STREAM_STATUS_WITHIN_VT100 ||
			status->status == TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM) // Within an escape sequence
	{
		if(c >= '0' && c <= '9') // Parameter value
		{
			if(status->parameterCount == 0)
			{
				status->parameterCount = 1;
			}

			if(status->parameterCount <= TERMINAL_STREAM_CONTROL_MAX_PARAMETERS)
			{
				status->parameters[status->parameterCount - 1] = status->parameters[status->parameterCount - 1] * 10;
				status->parameters[status->parameterCount - 1] += c - '0';

				// Illegal number of parameters is skipped
			}
		}
		else if(c == ';') // Parameter seperator
		{
			status->parameterCount++;
		}
		else // Finish character
		{
			status->controlCharacter = c;

			if(status->status == TERMINAL_STREAM_STATUS_WITHIN_VT100)
			{
				terminalProcessSequenceVt100(status);
			}
			else if(status->status == TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM)
			{
				terminalProcessSequenceGhostterm(status);
			}

			// reset status
			status->parameterCount = 0;
			for(int i = 0; i < TERMINAL_STREAM_CONTROL_MAX_PARAMETERS; i++)
			{
				status->parameters[i] = 0;
			}
			status->status = TERMINAL_STREAM_STATUS_TEXT;
		}
	}
}

void terminalProcessSequenceVt100(stream_control_status_t* status)
{
	switch(status->controlCharacter)
	{
	case 'A': // Cursor up
		screen->setCursor(screen->getCursorX(), screen->getCursorY() - status->parameters[0]);
		break;

	case 'B': // Cursor down
		screen->setCursor(screen->getCursorX(), screen->getCursorY() + status->parameters[0]);
		break;

	case 'C': // Cursor forward
		screen->setCursor(screen->getCursorX() + status->parameters[0], screen->getCursorY());
		break;

	case 'D': // Cursor back
		screen->setCursor(screen->getCursorX() - status->parameters[0], screen->getCursorY());
		break;

	case 'm': // Mode setting
		for(int i = 0; i < status->parameterCount; i++)
		{
			int param = status->parameters[i];

			if(param == 0) // Reset
			{
				screen->setColorBackground(SC_BLACK);
				screen->setColorForeground(SC_WHITE);
			}
			else if(param >= 30 && param < 40) // Foreground color
			{
				screen->setColorForeground(terminalColorFromVt100(param - 30));
			}
			else if(param >= 40 && param < 50) // Background color
			{
				screen->setColorBackground(terminalColorFromVt100(param - 40));
			}
		}

		break;

	case 'J': // Clearing
		if(status->parameterCount == 1)
		{
			// Clear the entire screen
			if(status->parameters[0] == 2)
			{
				screen->clean();
			}
		}
		break;

	case 'f': // Reposition cursor
		screen->setCursor(status->parameters[1], status->parameters[0]);
		break;

		// Cursor queries
	case 'n': // Query position

		if(status->parameters[0] == 6)
		{
			std::stringstream response;
			response << (char) G_TERMKEY_ESC << "[" << screen->getCursorY() << ";" << screen->getCursorX() << "R";
			auto responseStr = response.str();
			write(shellStdin, responseStr.c_str(), responseStr.size());
		}
		break;

	case 'r': // Set scroll area
		if(status->parameterCount == 0)
		{
			screen->setScrollAreaScreen();
		}
		else
		{
			screen->setScrollArea(status->parameters[0], status->parameters[1]);
		}
		break;

	case 'S': // Scroll Scrolling Region Up
		screen->scroll(status->parameters[0]);
		break;

	case 'T': // Scroll Scrolling Region Down
		screen->scroll(-status->parameters[0]);
		break;
	}
}

void terminalProcessSequenceGhostterm(stream_control_status_t* status)
{
	switch(status->controlCharacter)
	{
	case 'm': // Change mode
		inputMode = (g_terminal_mode) status->parameters[0];
		break;

	case 'e': // Change echo
		inputEcho = (status->parameters[0] == 1);
		break;

	case 'i': // Screen info
	{
		std::stringstream response;
		response << (char) G_TERMKEY_ESC << "{" << screen->getWidth() << ";" << screen->getHeight() << "i";
		terminalWriteToShell(response.str().c_str());
		break;
	}

	case 'p': // Put char
		screen->write(status->parameters[0]);
		break;

	case 'c': // Process control
		controlProcess = status->parameters[0];
		break;

		// Various other controls
	case 'C':
		// show/hide cursor
		if(status->parameters[0] == 0)
		{
			screen->setCursorVisible(status->parameters[1]);
		}
		break;
	}
}

screen_color_t terminalColorFromVt100(int color)
{
	switch(color)
	{
	case VT100_COLOR_BLACK:
		return SC_BLACK;
	case VT100_COLOR_BLUE:
		return SC_BLUE;
	case VT100_COLOR_CYAN:
		return SC_CYAN;
	case VT100_COLOR_GREEN:
		return SC_GREEN;
	case VT100_COLOR_MAGENTA:
		return SC_MAGENTA;
	case VT100_COLOR_RED:
		return SC_RED;
	case VT100_COLOR_WHITE:
		return SC_WHITE;
	case VT100_COLOR_YELLOW:
		return SC_YELLOW;
	case VT100_COLOR_GRAY:
		return SC_LGRAY;
	}
	return SC_WHITE;
}
