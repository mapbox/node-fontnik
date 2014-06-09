/*****************************************************************************
 *
 * Copyright (C) 2014 Mapbox
 *
 * This file is part of Fontnik.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#ifndef FONTNIK_GUARDED_MAP_HPP
#define FONTNIK_GUARDED_MAP_HPP

// stl
#include <memory>
#include <map>
#include <mutex>

namespace fontnik {

template <class K, class V, class Compare = std::less<K>,
          class Allocator = std::allocator<std::pair<const K, V> > >

class guarded_map {
  private:
    typedef std::map<K, V, Compare, Allocator> map_type;
    typedef std::lock_guard<std::mutex> lock_guard_type;

    map_type map;
    std::mutex mutex;

  public:
    typedef typename map_type::const_iterator const_iterator;

    bool empty() {
      lock_guard_type guard(this->mutex);
      return this->map.empty();
    }

    const_iterator cbegin() {
      lock_guard_type guard(this->mutex);
      return this->map.cbegin();
    }

    const_iterator cend() {
      lock_guard_type guard(this->mutex);
      return this->map.cend();
    }

    std::pair<const_iterator, bool> emplace(K key, V value) {
      lock_guard_type guard(this->mutex);
      return this->map.emplace(key, value);
    }

    const_iterator find(K key) {
      lock_guard_type guard(this->mutex);
      return this->map.find(key);
    }

    std::pair<const_iterator, bool> insert(K key, V value) {
      lock_guard_type guard(this->mutex);
      return this->map.emplace(key, value);
    }

    size_t size() {
      lock_guard_type guard(this->mutex);
      return this->map.size();
    }
};

} // ns fontnik

#endif // FONTNIK_GUARDED_MAP__HPP
