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

#include "stdio.h"
#include "errno.h"
#include "string.h"

/**
 *
 */
typedef struct {
	char* out;
	size_t maximum;
	size_t written;
	size_t produced;
} vcbprintf_vsnprintf_callback_data_t;

/**
 *
 */
static ssize_t vcbprintf_vsnprintf_callback(void* param, const char* str,
		size_t len) {

	vcbprintf_vsnprintf_callback_data_t* data =
			(vcbprintf_vsnprintf_callback_data_t*) param;

	if (data->produced < data->maximum) {
		size_t space = data->maximum - data->produced;
		size_t writable = (len < space) ? len : space;
		memcpy(data->out + data->produced, str, writable);
		data->written += writable;
	}

	data->produced += len;
	return len;
}

/**
 *
 */
int vsnprintf(char* s, size_t n, const char* format, va_list arg) {
	vcbprintf_vsnprintf_callback_data_t info;
	info.out = s;
	info.maximum = n ? n - 1 : 0;
	info.written = 0;
	info.produced = 0;
	int result = vcbprintf(&info, vcbprintf_vsnprintf_callback, format, arg);
	if (n) {
		info.out[info.written] = '\0';
	}
	return result;
}
