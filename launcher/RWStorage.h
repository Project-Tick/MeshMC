/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <QWriteLocker>
#include <QReadLocker>
#include <QMap>
#include <QSet>

template <typename K, typename V> class RWStorage
{
  public:
	void add(K key, V value)
	{
		QWriteLocker l(&lock);
		cache[key] = value;
		stale_entries.remove(key);
	}
	V get(K key)
	{
		QReadLocker l(&lock);
		if (cache.contains(key)) {
			return cache[key];
		} else
			return V();
	}
	bool get(K key, V& value)
	{
		QReadLocker l(&lock);
		if (cache.contains(key)) {
			value = cache[key];
			return true;
		} else
			return false;
	}
	bool has(K key)
	{
		QReadLocker l(&lock);
		return cache.contains(key);
	}
	bool stale(K key)
	{
		QReadLocker l(&lock);
		if (!cache.contains(key))
			return true;
		return stale_entries.contains(key);
	}
	void setStale(K key)
	{
		QWriteLocker l(&lock);
		if (cache.contains(key)) {
			stale_entries.insert(key);
		}
	}
	void clear()
	{
		QWriteLocker l(&lock);
		cache.clear();
		stale_entries.clear();
	}

  private:
	QReadWriteLock lock;
	QMap<K, V> cache;
	QSet<K> stale_entries;
};
