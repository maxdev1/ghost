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

#ifndef __UTILS_HASHMAP_STRING__
#define __UTILS_HASHMAP_STRING__

#include "kernel/utils/hashmap.hpp"

const char* hashmapKeyCopyString(const char* key);
int hashmapKeyHashString(const char* key);
void hashmapKeyFreeString(const char* key);
bool hashmapKeyEqualsString(const char* k1, const char* k2);

template<typename V>
g_hashmap<const char*, V>* hashmapCreateString(int bucketCount) {
	g_hashmap<const char*, V>* map = hashmapInternalCreate<const char*, V>(bucketCount);
	map->keyCopy = hashmapKeyCopyString;
	map->keyHash = hashmapKeyHashString;
	map->keyFree = hashmapKeyFreeString;
	map->keyEquals = hashmapKeyEqualsString;
	return map;
}

#endif
