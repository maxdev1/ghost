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

#include "stdint.h"
#include "stdlib.h"

static inline void swapElement(uint8_t* array, int a, int b, size_t size)
{
	if(a == b)
		return;

	uint8_t* va = array + a * size;
	uint8_t* vb = array + b * size;
	uint8_t temp;
	for(size_t i = 0; i < size; i++)
	{
		temp = va[i];
		va[i] = vb[i];
		vb[i] = temp;
	}
}

static void qsortr(uint8_t* array, size_t size, int (*comparator)(const void*, const void*), int low, int high)
{
	if(low >= high)
		return;

	int p = high;
	int g = (low - 1);

	for(int s = low; s < high; s++)
	{
		if(comparator(array + s * size, array + p * size) <= 0)
		{
			++g;
			swapElement(array, g, s, size);
		}
	}

	swapElement(array, g + 1, high, size);
	qsortr(array, size, comparator, low, g);
	qsortr(array, size, comparator, g + 1, high);
}

void qsort(void* array, size_t elements, size_t size, int (*comparator)(const void*, const void*))
{
	qsortr((uint8_t*) array, size, comparator, 0, elements - 1);
}
