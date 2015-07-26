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
#include "stdio.h"
#include "stdint.h"

/**
 *
 */
static void swap(uint8_t* x, uint8_t* y, size_t element_size) {

	char* a = (char*) x;
	char* b = (char*) y;
	char c;
	while (element_size--) {
		c = *a;
		*a++ = *b;
		*b++ = c;
	}
}

/**
 *
 */
static void sort(uint8_t* arr, size_t el_sz,
		int (*comparator)(const void*, const void*), int begin, int end) {

	if (end > begin) {
		uint8_t* pivot = arr + begin;

		int loff = begin + el_sz;
		int roff = end;

		while (loff < roff) {
			if (comparator(arr + loff, pivot) <= 0) {
				loff += el_sz;

			} else if (comparator(arr + roff, pivot) > 0) {
				roff -= el_sz;

			} else if (loff < roff) {
				swap(arr + loff, arr + roff, el_sz);
			}
		}

		loff -= el_sz;

		swap(arr + begin, arr + loff, el_sz);
		sort(arr, el_sz, comparator, begin, loff);
		sort(arr, el_sz, comparator, roff, end);
	}
}

/**
 *
 */
void qsort(void* array, size_t num_elements, size_t element_size,
		int (*comparator)(const void*, const void*)) {
	sort((uint8_t*) array, element_size, comparator, 0,
			num_elements * element_size);
}
