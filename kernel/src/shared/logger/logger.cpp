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

#include "shared/logger/logger.hpp"

#include "shared/utils/string.hpp"
#include "shared/system/serial_port.hpp"
#include "shared/system/bios_data_area.hpp"
#include "shared/video/console_video.hpp"
#include "shared/debug/debug_interface.hpp"

static const uint32_t LOGGER_HEADER_WIDTH = 10;

static bool logSerial = false;
static bool logVideo = true;

void loggerEnableSerial(bool serial)
{
	logSerial = serial;
}

void loggerEnableVideo(bool video)
{
	logVideo = video;
}

void loggerPrintFormatted(const char *message_const, va_list valist)
{
	char *message = (char *) message_const;

	uint16_t headerColor = 0x07;

	while(*message)
	{
		if(*message != '%')
		{
			loggerPrintCharacter(*message);
			++message;
			continue;
		}

		++message;
		if(*message == 'i')
		{ // integer
			int32_t val = va_arg(valist, int32_t);
			loggerPrintNumber(val, 10);

		} else if(*message == 'h' || *message == 'x')
		{ // positive hex number
			uint32_t val = va_arg(valist, uint32_t);
			loggerPrintPlain("0x");
			loggerPrintNumber(val, 16);

		} else if(*message == 'b')
		{ // boolean
			int val = va_arg(valist, int);
			loggerPrintPlain((const char *) (val ? "true" : "false"));

		} else if(*message == 'c')
		{ // char
			int val = va_arg(valist, int);
			loggerPrintCharacter((char) val);

		} else if(*message == 's')
		{ // string
			char* val = va_arg(valist, char*);
			loggerPrintPlain(val);

		} else if(*message == '#')
		{ // indented printing
			for(uint32_t i = 0; i < LOGGER_HEADER_WIDTH + 2; i++)
			{
				loggerPrintCharacter(' ');
			}

		} else if(*message == '!')
		{ // indented header printing
			char *val = va_arg(valist, char *);
			uint32_t headerlen = stringLength(val);

			if(headerlen < LOGGER_HEADER_WIDTH)
			{
				for(uint32_t i = 0; i < LOGGER_HEADER_WIDTH - headerlen; i++)
				{
					loggerPrintCharacter(' ');
				}
			}

			consoleVideoSetColor(headerColor);
			loggerPrintPlain(val);
			consoleVideoSetColor(0x0F);

		} else if(*message == '%')
		{ // escaped %
			loggerPrintCharacter(*message);

		} else if(*message == '*')
		{ // header color change
			headerColor = (uint16_t) (va_arg(valist, int));
		}
		++message;
	}
}

void loggerPrintNumber(uint32_t number, uint16_t base)
{

	// Remember if negative
	uint8_t negative = 0;
	if(base == 10)
	{
		negative = ((int32_t) number) < 0;

		if(negative)
		{
			number = -number;
		}
	}

	// Write chars in reverse order, not nullterminated
	char revbuf[32];

	char *cbufp = revbuf;
	int len = 0;
	do
	{
		*cbufp++ = "0123456789ABCDEF"[number % base];
		++len;
		number /= base;
	} while(number);

	// If base is 16, write 0's until 8
	if(base == 16)
	{
		while(len < 8)
		{
			*cbufp++ = '0';
			++len;
		}
	}

	// Reverse buffer
	char buf[len + 1];
	for(int i = 0; i < len; i++)
	{
		buf[i] = revbuf[len - i - 1];
	}
	buf[len] = 0;

	// Print number
	if(negative)
	{
		loggerPrintCharacter('-');
	}
	loggerPrintPlain(buf);
}

void loggerPrintPlain(const char* message_const)
{
	char* message = (char*) message_const;
	while(*message)
	{
		loggerPrintCharacter(*message++);
	}
}

void loggerPrintCharacter(char c)
{
	if(logVideo)
	{
		consoleVideoPrint(c);
	}
	if(logSerial)
	{
		debugInterfaceWriteLogCharacter(c);
	}
}
