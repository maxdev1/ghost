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

#include "../src-common/io.hpp"
#include <fcntl.h>

int main(int argc, char** argv)
{
	int first = 1;
	bool options = true;
	if(first < argc && strcmp(argv[first], "--") == 0)
	{
		options = false;
		++first;
	}
	else if(first < argc && (strcmp(argv[first], "-h") == 0 || strcmp(argv[first], "--help") == 0))
	{
		printf("Usage: read [FILE...]\nPrint files to standard output. No files or '-' reads standard input.\n");
		return 0;
	}

	if(first == argc)
		return coreTransfer(STDIN_FILENO, STDOUT_FILENO, "read", "standard input", "standard output") ? 1 : 0;

	int result = 0;
	for(int i = first; i < argc; ++i)
	{
		const char* path = argv[i];
		if(options && path[0] == '-' && strcmp(path, "-") != 0)
		{
			fprintf(stderr, "read: unknown option '%s' (use -- before filenames beginning with '-')\n", path);
			return 1;
		}
		bool standardInput = strcmp(path, "-") == 0;
		int input = standardInput ? STDIN_FILENO : open(path, O_RDONLY);
		if(input < 0)
		{
			coreReportError("read", "open", path);
			result = 1;
			continue;
		}
		int transfer = coreTransfer(input, STDOUT_FILENO, "read", path, "standard output");
		if(transfer)
			result = 1;
		if(!standardInput && coreClose(input, "read", path))
			result = 1;
		if(transfer == 2)
			break;
	}
	return result;
}
