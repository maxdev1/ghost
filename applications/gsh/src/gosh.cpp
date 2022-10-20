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

#include <dirent.h>
#include <gosh.hpp>
#include <iostream>
#include <libterminal/terminal.hpp>
#include <parser.hpp>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

char* cwdbuf = 0;

std::vector<std::string> gshAutocomplete(std::string toComplete)
{
	std::string cwd(cwdbuf);
	std::string startDir = cwd;

	auto firstSlash = toComplete.find_first_of('/');
	auto lastSlash = toComplete.find_last_of('/');
	std::string prepend;
	if(firstSlash == 0)
	{
		startDir = toComplete.substr(0, lastSlash);
		prepend = startDir + "/";
		toComplete = toComplete.substr(lastSlash + 1);
	}
	else if(lastSlash != std::string::npos)
	{
		startDir += "/" + toComplete.substr(0, lastSlash);
		prepend = toComplete.substr(0, lastSlash) + "/";
		toComplete = toComplete.substr(lastSlash + 1);
	}

	std::vector<std::string> completions;
	DIR* dir = opendir(startDir.c_str());
	if(dir)
	{
		dirent* ent;
		while(ent = readdir(dir))
		{
			std::string entname(ent->d_name);
			if(entname.rfind(toComplete, 0) == 0)
			{
				DIR* subdir = opendir((startDir + "/" + entname).c_str());
				bool isdir = false;
				if(subdir)
				{
					isdir = true;
					closedir(subdir);
				}

				completions.push_back(prepend + entname + (isdir ? "/" : ""));
			}
		}
		closedir(dir);
	}
	return completions;
}

bool gshReadInputLine(std::string& line)
{
	g_terminal::setMode(G_TERMINAL_MODE_RAW);
	g_terminal::setEcho(false);

	int caret = 0;

	while(true)
	{
		int c = g_terminal::getChar();
		if(c == -1)
		{
			klog("getc returned -1");
			return false;
		}

		if(c == G_TERMKEY_BACKSPACE)
		{
			if(line.size() > 0 && caret > 0)
			{
				char deleted = line.at(caret - 1);

				auto pos = g_terminal::getCursor();
				auto afterCaret = line.substr(caret);
				line = line.substr(0, caret - 1) + afterCaret;
				caret--;

				if(deleted == '\t')
				{
					pos.x -= 4;
					afterCaret += "    ";
				}
				else
				{
					pos.x--;
					afterCaret += ' ';
				}
				g_terminal::setCursor(pos);
				for(char c : afterCaret)
				{
					std::cout << c;
				}
				g_terminal::setCursor(pos);
			}
		}
		else if(c == '\t' || c == G_TERMKEY_STAB /* TODO implement completion */)
		{
			auto beforeCaret = line.substr(0, caret);
			auto afterCaret = line.substr(caret);

			std::string toComplete = beforeCaret;
			auto space = toComplete.find_last_of(' ');
			if(space != std::string::npos)
			{
				toComplete = toComplete.substr(space + 1);
			}

			std::vector<std::string> completions = gshAutocomplete(toComplete);
			if(completions.size() > 1)
			{
				// TODO
			}
			else if(completions.size() > 0)
			{
				auto completion = completions.at(0);
				caret = caret - toComplete.size();
				line = line.substr(0, caret) + completion + afterCaret;
				g_terminal::moveCursorBack(toComplete.size());

				auto pos = g_terminal::getCursor();
				caret += completion.size();
				pos.x += completion.size();

				std::cout << completion << afterCaret;
				g_terminal::setCursor(pos);
			}
		}
		else if(c == G_TERMKEY_ENTER)
		{
			std::cout << '\n';
			break;
		}
		else if(c == G_TERMKEY_LEFT)
		{
			if(caret > 0)
			{
				char beforeCaret = line.at(caret - 1);

				caret--;
				if(beforeCaret == '\t')
				{
					g_terminal::moveCursorBack(4);
				}
				else
				{
					g_terminal::moveCursorBack(1);
				}
			}
		}
		else if(c == G_TERMKEY_RIGHT)
		{
			if(caret < line.size())
			{
				char atCaret = line.at(caret);
				caret++;
				if(atCaret == '\t')
				{
					g_terminal::moveCursorForward(4);
				}
				else
				{
					g_terminal::moveCursorForward(1);
				}
			}
		}
		else if(c < 0x100)
		{
			auto pos = g_terminal::getCursor();
			std::cout << (char) c;

			auto afterCaret = line.substr(caret);
			line = line.substr(0, caret) + (char) c + afterCaret;
			caret++;

			pos.x++;
			g_terminal::setCursor(pos);
			for(char c : afterCaret)
			{
				std::cout << c;
			}
			g_terminal::setCursor(pos);
		}
	}

	return true;
}

/**
 *
 */
bool gshFileExists(std::string path)
{

	FILE* f;
	if((f = fopen(path.c_str(), "r")) != NULL)
	{
		fclose(f);
		return true;
	}
	return false;
}

/**
 *
 */
std::string gshFindProgram(std::string cwd, std::string name)
{
	// check for match with cwd
	std::string path = cwd + "/" + name;
	if(gshFileExists(path))
	{
		return path;
	}

	// check for full path
	if(gshFileExists(name))
	{
		return name;
	}

	// check for /applications folder
	path = "/applications/" + name;
	if(gshFileExists(path))
	{
		return path;
	}

	// last chance - check for .bin extension
	if(name.length() < 4 || name.substr(name.length() - 4) != ".bin")
	{
		return gshFindProgram(cwd, name + ".bin");
	}

	// nothing found
	return name;
}

bool gshHandleBuiltin(program_call_t* call)
{
	std::string cwd(cwdbuf);

	if(call->program == "cd")
	{
		if(call->arguments.size() == 1)
		{
			std::string newdir = call->arguments.at(0);
			if(newdir.find_first_of("/") != 0)
			{
				newdir = cwd + "/" + newdir;
			}

			DIR* dir = opendir(newdir.c_str());
			if(dir)
			{
				closedir(dir);
				g_set_working_directory(newdir.c_str());
			}
			else
			{
				std::cerr << "Directory not found" << std::endl;
			}
		}
		else
		{
			std::cerr << "Usage:\tcd /path/to/target" << std::endl;
		}
		return true;
	}

	if(call->program == "clear" || call->program == "cls")
	{
		g_terminal::clear();
		g_term_cursor_position pos;
		pos.x = 0;
		pos.y = 0;
		g_terminal::setCursor(pos);
		return true;
	}

	if(call->program == "which")
	{
		if(call->arguments.size() == 1)
		{
			std::cout << gshFindProgram(cwd, call->arguments.at(0)) << std::endl;
		}
		else
		{
			std::cerr << "Usage:\twhich command" << std::endl;
		}
		return true;
	}

	if(call->program == "exit")
	{
		exit(0);
	}

	return false;
}

void gshProcessExpression(pipe_expression_t* pipeexpr)
{
	g_fd lastPipeRead = G_FD_NONE;
	g_pid headProcessId = G_PID_NONE;
	g_pid tailProcessId = G_PID_NONE;
	bool success = false;

	auto calls = pipeexpr->calls.size();
	for(int callIndex = 0; callIndex < calls; callIndex++)
	{
		program_call_t* call = pipeexpr->calls[callIndex];

		// builtin calls (only allowed as single call)
		if(calls == 1 && callIndex == 0 && gshHandleBuiltin(call))
		{
			break;
		}

		// concatenate arguments to one argument string
		std::stringstream argstream;
		bool first = true;
		for(auto arg : call->arguments)
		{
			if(first)
			{
				first = false;
			}
			else
			{
				argstream << (char) G_CLIARGS_SEPARATOR;
			}
			argstream << arg;
		}

		// create out pipe if necessary
		g_fd pipeWrite = G_FD_NONE;
		g_fd pipeRead = G_FD_NONE;
		if(calls > 1 && callIndex < calls - 1)
		{
			g_fs_pipe_status pipe_stat = g_pipe(&pipeWrite, &pipeRead);

			if(pipe_stat != G_FS_PIPE_SUCCESSFUL)
			{
				std::cerr << "failed to create output pipe when spawning '" << call->program << "'" << std::endl;
				// TODO clean up pipes?
				success = false;
				break;
			}
		}

		// decide how to set in/out/err file descriptors
		g_fd stdioIn[3];
		stdioIn[STDERR_FILENO] = STDERR_FILENO;
		if(callIndex == 0)
			stdioIn[STDIN_FILENO] = STDIN_FILENO;
		else
			stdioIn[STDIN_FILENO] = lastPipeRead;
		if((calls == 1 && callIndex == 0) || callIndex == calls - 1)
			stdioIn[STDOUT_FILENO] = STDOUT_FILENO;
		else
			stdioIn[STDOUT_FILENO] = pipeWrite;

		// do spawning
		g_pid pidCurrent;
		g_fd stdioOut[3];
		g_spawn_status status = g_spawn_poi(
			gshFindProgram(std::string(cwdbuf), call->program).c_str(),
			argstream.str().c_str(), cwdbuf, G_SECURITY_LEVEL_APPLICATION, &pidCurrent, stdioOut, stdioIn);

		// check result
		if(status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			if(headProcessId == G_TID_NONE)
				headProcessId = pidCurrent;
			tailProcessId = pidCurrent;
			success = true;

			// close write end in this process
			if(pipeWrite != G_FD_NONE)
				g_close(pipeWrite);

			if(lastPipeRead != G_FD_NONE)
				g_close(lastPipeRead);

			// remember for next process
			lastPipeRead = pipeRead;
		}
		else
		{
			success = false;
			// error during one spawn
			// TODO clean up pipes
			std::cout << (char) 27 << "[31m";
			if(status == G_SPAWN_STATUS_FORMAT_ERROR)
			{
				std::cout << call->program << ": invalid binary format"
						  << std::endl;
			}
			else if(status == G_SPAWN_STATUS_IO_ERROR)
			{
				std::cout << call->program << ": command not found"
						  << std::endl;
			}
			else
			{
				std::cout << call->program
						  << ": unknown error during program execution"
						  << std::endl;
			}
			std::cout << (char) 27 << "[0m";
			std::flush(std::cout);
			break;
		}
	}

	if(success)
	{
		// join into the last process
		g_terminal::setControlProcess(tailProcessId);
		g_join(tailProcessId);
		g_terminal::setControlProcess(0);
	}
}

/**
 *
 */
int main(int argc, char* argv[])
{
	cwdbuf = new char[G_PATH_MAX];
	g_terminal::setCursor(g_term_cursor_position(0, 0));

	while(true)
	{

		// print cwd
		std::cout << (char) 27 << "[38m";
		if(g_get_working_directory(cwdbuf) == G_GET_WORKING_DIRECTORY_SUCCESSFUL)
		{
			std::cout << cwdbuf;
		}
		else
		{
			std::cout << "?";
		}
		std::cout << '>';
		std::cout << (char) 27 << "[0m";
		std::flush(std::cout);

		g_terminal::setCursor(g_terminal::getCursor());

		std::string line;
		if(!gshReadInputLine(line))
			break;

		// switch to normal input mode
		g_terminal::setMode(G_TERMINAL_MODE_DEFAULT);
		g_terminal::setEcho(true);

		parser_t cmdparser(line);
		pipe_expression_t* pipeexpr;
		if(!cmdparser.pipe_expression(&pipeexpr))
		{
			continue;
		}
		gshProcessExpression(pipeexpr);
	}

	delete cwdbuf;
}
