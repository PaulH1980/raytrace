#pragma once
#include "FrontEndDef.h"
#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>
namespace RayTrace
{
	template<typename Key, typename Value>
	class LRUCache {
	public:
		typedef typename std::pair<Key, Value> KeyValuePair;
		typedef typename std::list<KeyValuePair>::iterator ListIterator;

		LRUCache(size_t _size) :
			m_maxSize(_size) {
		}

		void Insert(const Key& key, const Value& value) {
			auto it = m_itemMap.find(key);
			m_itemList.push_front(KeyValuePair(key, value));
			if (it != m_itemMap.end()) {
				m_itemList.erase(it->second);
				m_itemMap.erase(it);
			}
			m_itemMap[key] = m_itemList.begin();

			if (m_itemMap.size() > m_maxSize) {
				auto last = m_itemList.end();
				last--;
				m_itemMap.erase(last->first);
				m_itemList.pop_back();
			}
		}

		const Value& GetValue(const Key& key) {
			auto it = m_itemMap.find(key);
			if (it == m_itemMap.end()) {
				throw std::exception("Key Not Found");
			}
			else {
				m_itemList.splice(m_itemList.begin(), m_itemList, it->second);
				return it->second->second;
			}
		}

		bool Contains(const Key& key) const {
			return m_itemMap.find(key) != m_itemMap.end();
		}

		size_t GetSize() const {
			return m_itemMap.size();
		}

	private:
		std::list<KeyValuePair> m_itemList;
		std::unordered_map<Key, ListIterator> m_itemMap;
		size_t m_maxSize;
	};
}

