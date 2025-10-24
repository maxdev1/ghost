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

#include "stdlib.h"
#include "errno.h"
#include "stdio.h"

/**
 *
 */
void* bsearch(const void* value, const void* array, size_t num_elements,
              size_t size, int (*comparator)(const void*, const void*))
{
	size_t low = 0;
	size_t high = num_elements;

	while(low < high)
	{
		size_t mid = low + (high - low) / 2;
		const void* elem = (const char*) array + mid * size;

		int cmp = comparator(value, elem);
		if(cmp < 0)
			high = mid;
		else if(cmp > 0)
			low = mid + 1;
		else
			return (void*) elem;
	}

	return NULL;
}
