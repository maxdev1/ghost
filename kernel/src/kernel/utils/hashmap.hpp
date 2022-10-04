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

#ifndef __UTILS_HASHMAP__
#define __UTILS_HASHMAP__

#include "shared/system/mutex.hpp"
#include "kernel/memory/memory.hpp"

template<typename K, typename V>
struct g_hashmap_entry
{
	K key;
	V value;
	g_hashmap_entry* next;
};

template<typename K, typename V>
struct g_hashmap
{
	g_mutex lock;

	g_hashmap_entry<K, V>** buckets;
	int bucketCount;

	K (*keyCopy)(K k);
	int (*keyHash)(K k);
	void (*keyFree)(K k);
	bool (*keyEquals)(K k1, K k2);
};

template<typename K, typename V>
g_hashmap<K, V>* hashmapInternalCreate(int bucketCount)
{

	g_hashmap<K, V>* map = (g_hashmap<K, V>*) heapAllocate(sizeof(g_hashmap<K, V> ));
	map->bucketCount = bucketCount;
	map->buckets = (g_hashmap_entry<K, V>**) heapAllocate(sizeof(g_hashmap_entry<K, V>*) * bucketCount);

	mutexInitialize(&map->lock);

	for(int i = 0; i < bucketCount; i++)
	{
		map->buckets[i] = 0;
	}
	return map;
}

template<typename K, typename V>
void hashmapPut(g_hashmap<K, V>* map, K key, V value)
{

	mutexAcquire(&map->lock);

	auto* entry = hashmapGetEntry(map, key);
	if(entry)
	{
		entry->value = value;
	} else
	{
		int bucket = map->keyHash(key) % map->bucketCount;
		auto* newEntry = (g_hashmap_entry<K, V>*) heapAllocate(sizeof(g_hashmap_entry<K, V> ));
		newEntry->key = map->keyCopy(key);
		newEntry->value = value;
		newEntry->next = map->buckets[bucket];
		map->buckets[bucket] = newEntry;
	}

	mutexRelease(&map->lock);
}

template<typename K, typename V>
g_hashmap_entry<K, V>* hashmapGetEntry(g_hashmap<K, V>* map, K key)
{

	mutexAcquire(&map->lock);

	int hash = map->keyHash(key);
	auto* entry = map->buckets[hash % map->bucketCount];
	while(entry)
	{
		if(map->keyEquals(entry->key, key))
		{
			break;
		}
		entry = entry->next;
	}

	mutexRelease(&map->lock);
	return entry;
}

template<typename K, typename V>
V hashmapGet(g_hashmap<K, V>* map, K key, V def)
{

	mutexAcquire(&map->lock);
	g_hashmap_entry<K, V>* entry = hashmapGetEntry(map, key);

	V value;
	if(entry)
	{
		value = entry->value;
	} else
	{
		value = def;
	}
	mutexRelease(&map->lock);
	return value;
}

template<typename K, typename V>
void hashmapRemove(g_hashmap<K, V>* map, K key)
{

	mutexAcquire(&map->lock);

	int bucket = map->keyHash(key) % map->bucketCount;
	auto* entry = map->buckets[bucket];
	g_hashmap_entry<K, V>* previous = 0;
	while(entry)
	{
		if(map->keyEquals(entry->key, key))
		{
			if(previous)
			{
				previous->next = entry->next;
			} else
			{
				map->buckets[bucket] = entry->next;
			}
			map->keyFree(entry->key);
			heapFree(entry);
			break;
		}
		previous = entry;
		entry = entry->next;
	}

	mutexRelease(&map->lock);
}

template<typename K, typename V>
struct g_hashmap_iterator
{
	int bucket;
	g_hashmap_entry<K, V>* current;
	g_hashmap<K, V>* map;
};

template<typename K, typename V>
g_hashmap_iterator<K, V> hashmapIteratorStart(g_hashmap<K, V>* map)
{
	mutexAcquire(&map->lock);

	g_hashmap_iterator<K, V> iter;
	iter.bucket = 0;
	iter.current = 0;
	iter.map = map;
	return iter;
}

template<typename K, typename V>
bool hashmapIteratorHasNext(g_hashmap_iterator<K, V>* iter)
{
	if(iter->current && iter->current->next)
	{
		return true;

	} else
	{
		int startBucket = iter->current ? iter->bucket + 1 : iter->bucket;
		for(int i = startBucket; i < iter->map->bucketCount; i++)
		{
			if(iter->map->buckets[i])
			{
				return true;
			}
		}
	}
	return false;
}

template<typename K, typename V>
g_hashmap_entry<K, V>* hashmapIteratorNext(g_hashmap_iterator<K, V>* iter)
{
	if(iter->current && iter->current->next)
	{
		iter->current = iter->current->next;
		return iter->current;

	} else
	{
		int startBucket = iter->current ? iter->bucket + 1 : iter->bucket;
		for(int i = startBucket; i < iter->map->bucketCount; i++)
		{
			if(iter->map->buckets[i])
			{
				iter->current = iter->map->buckets[i];
				iter->bucket = i;
				return iter->current;
			}
		}
	}
	return 0;
}

template<typename K, typename V>
void hashmapIteratorEnd(g_hashmap_iterator<K, V>* iter)
{
	mutexRelease(&iter->map->lock);
}

// Implementation for primitive key types
#include <type_traits>

template<typename K, typename = typename std::enable_if<std::is_arithmetic<K>::value, K>::type>
K hashmapKeyCopyNumeric(K key)
{
	return key;
}

template<typename K, typename = typename std::enable_if<std::is_arithmetic<K>::value, K>::type>
int hashmapKeyHashNumeric(K key)
{
	return key < 0 ? -key : key;
}

template<typename K, typename = typename std::enable_if<std::is_arithmetic<K>::value, K>::type>
void hashmapKeyFreeNumeric(K key)
{
}

template<typename K, typename = typename std::enable_if<std::is_arithmetic<K>::value, K>::type>
bool hashmapKeyEqualsNumeric(K k1, K k2)
{
	return k1 == k2;
}

template<typename K, typename V, typename = typename std::enable_if<std::is_arithmetic<K>::value, K>::type>
g_hashmap<K, V>* hashmapCreateNumeric(int bucketCount)
{
	g_hashmap<K, V>* map = hashmapInternalCreate<K, V>(bucketCount);
	map->keyCopy = hashmapKeyCopyNumeric;
	map->keyHash = hashmapKeyHashNumeric;
	map->keyFree = hashmapKeyFreeNumeric;
	map->keyEquals = hashmapKeyEqualsNumeric;
	return map;
}

#endif
