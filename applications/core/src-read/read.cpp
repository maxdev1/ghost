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

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage:\tread <filename>\n");
		return 1;
	}
	const char* filepath = argv[1];
	const int file = open(filepath, O_RDONLY);
	if(file < 0)
	{
		fprintf(stderr, "Cannot open: %s\n", filepath);
		return 1;
	}
	uint8_t buffer[16384];
	int result = 0;
	for(;;)
	{
		ssize_t count = read(file, buffer, sizeof(buffer));
		if(count < 0 && errno == EINTR)
			continue;
		if(count <= 0)
		{
			if(count < 0)
			{
				fprintf(stderr, "Cannot read: %s\n", filepath);
				result = 1;
			}
			break;
		}
		ssize_t offset = 0;
		while(offset < count)
		{
			ssize_t written = write(STDOUT_FILENO, buffer + offset, count - offset);
			if(written < 0 && errno == EINTR)
				continue;
			if(written <= 0)
			{
				fprintf(stderr, "Cannot write output\n");
				result = 1;
				break;
			}
			offset += written;
		}
		if(result)
			break;
	}
	close(file);
	return result;
}
