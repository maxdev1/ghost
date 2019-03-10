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

#include "kernel/utils/hashmap_string.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/utils/string.hpp"

const char* hashmapKeyCopyString(const char* key) {
	return stringDuplicate(key);
}

int hashmapKeyHashString(const char* key) {
	int hash = stringHash(key);
	if(hash < 0) hash = -hash;
	return hash;
}

void hashmapKeyFreeString(const char* key) {
	heapFree((void*) key);
}

bool hashmapKeyEqualsString(const char* k1, const char* k2) {
	return stringEquals(k1, k2);
}
