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

#ifndef GHOST_SHARED_UTILS_HASHMAP
#define GHOST_SHARED_UTILS_HASHMAP

#include <utils/hashable.hpp>

/**
 *
 */
template<typename K, typename V>
class g_hash_map {
public:

	/**
	 *
	 */
	class g_hash_map_entry {
	public:

		g_hash_map_entry(const K& key, const V& value) :
				key(key), value(value), next(0) {
		}

		K key;
		V value;
		g_hash_map_entry* next;
	};

	/**
	 *
	 */
	class g_hash_map_iterator {
	public:

		/**
		 *
		 */
		g_hash_map_iterator() :
				_map(0), _bucketIndex(0), _current(0) {
		}

		/**
		 *
		 */
		g_hash_map_iterator(g_hash_map* map) :
				_map(map), _bucketIndex(0), _current(0) {

			for (uint32_t i = 0; i < _map->_bucketCount; ++i) {
				if (_map->_buckets[i] != 0) {
					_bucketIndex = i;
					_current = _map->_buckets[i];
					break;
				}
			}
		}

		/**
		 *
		 */
		g_hash_map_iterator& operator++() {

			if (_current->next == 0) {
				_current = 0;

				while (++_bucketIndex < _map->_bucketCount) {
					if (_map->_buckets[_bucketIndex] != 0) {
						_current = _map->_buckets[_bucketIndex];
						break;
					}
				}
			} else {
				_current = _current->next;
			}

			return *this;
		}

		/**
		 *
		 */
		g_hash_map_entry* operator->() {

			return _current;
		}

		/**
		 *
		 */
		friend bool operator==(const g_hash_map_iterator& lhs, const g_hash_map_iterator& rhs) {

			return lhs._current == rhs._current;
		}

		/**
		 *
		 */
		friend bool operator!=(const g_hash_map_iterator& lhs, const g_hash_map_iterator& rhs) {

			return !(lhs == rhs);
		}

	private:

		g_hash_map* _map;
		uint32_t _bucketIndex;
		g_hash_map_entry* _current;
	};

	/**
	 *
	 */
	g_hash_map() :
			_size(0), _bucketCount(64), _buckets(new g_hash_map_entry*[_bucketCount]()) {
	}

	/**
	 *
	 */
	g_hash_map(const g_hash_map& rhs) = delete;

	/**
	 *
	 */
	~g_hash_map() {

		for (uint32_t i = 0; i < _bucketCount; ++i) {
			g_hash_map_entry* current = _buckets[i];

			while (current != 0) {
				g_hash_map_entry* next = current->next;
				delete current;
				current = next;
			}
		}

		delete[] _buckets;
	}

	/**
	 *
	 */
	g_hash_map& operator=(const g_hash_map& rhs) = delete;

	/**
	 *
	 */
	g_hash_map_iterator begin() {

		return g_hash_map_iterator(this);
	}

	/**
	 *
	 */
	g_hash_map_iterator end() {

		return g_hash_map_iterator();
	}

	/**
	 *
	 */
	g_hash_map_entry* get(const K& key) const {

		uint32_t bucketIndex = g_hashable::hashcode(key) % _bucketCount;

		g_hash_map_entry* entry = _buckets[bucketIndex];

		while (entry != 0) {
			if (entry->key == key) {
				return entry;
			}

			entry = entry->next;
		}

		return 0;
	}

	/**
	 *
	 */
	void put(const K& key, const V& value) {

		uint32_t bucketIndex = g_hashable::hashcode(key) % _bucketCount;

		g_hash_map_entry* entry = _buckets[bucketIndex];

		if (entry == 0) {
			_buckets[bucketIndex] = new g_hash_map_entry(key, value);
			++_size;
		} else {
			for (;;) {
				if (entry->key == key) {
					entry->value = value;
					++_size;
					break;
				} else if (entry->next == 0) {
					entry->next = new g_hash_map_entry(key, value);
					++_size;
					break;
				}

				entry = entry->next;
			}
		}
	}

	/**
	 *
	 */
	void remove(const K& key) {

		uint32_t bucketIndex = g_hashable::hashcode(key) % _bucketCount;

		g_hash_map_entry* current = _buckets[bucketIndex];
		g_hash_map_entry* previous = 0;

		while (current != 0) {
			if (current->key == key) {
				if (previous == 0) {
					_buckets[bucketIndex] = current->next;
				} else {
					previous->next = current->next;
				}

				delete current;
				--_size;
				break;
			}

			previous = current;
			current = current->next;
		}
	}

	/**
	 *
	 */
	uint32_t size() const {
		return _size;
	}

private:
	uint32_t _size;
	uint32_t _bucketCount;
	g_hash_map_entry** _buckets;
};

#endif
