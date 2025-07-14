// File: inc/cache_engine/policies/storage/hash_list_storage.hpp

#pragma once

#include <list>
#include <stdexcept>
#include <unordered_map>

namespace cache_engine
{
	namespace policies
	{
		namespace storage
		{
			/**
			 * @brief Hash list storage policy optimized for LRU/MRU algorithms
			 * @details Provides O(1) insert, find, erase, and move operations using
			 *          std::list for ordering and std::unordered_map for fast lookup
			 * @tparam key_t Key type
			 * @tparam value_t Value type
			 */
			template <typename key_t, typename value_t> class hash_list_storage
			{
			  public:
				using self_t				= hash_list_storage<key_t, value_t>;
				using list_iterator_t		= typename std::list<key_t>::iterator;
				using value_iterator_pair_t = std::pair<value_t, list_iterator_t>;

			  private:
				std::list<key_t> m_list;
				std::unordered_map<key_t, value_iterator_pair_t> m_map;
				std::size_t m_capacity;

			  public:
				~hash_list_storage() = default;

				explicit hash_list_storage(std::size_t p_capacity) : m_capacity(p_capacity) {}

				hash_list_storage(const self_t&)		 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				hash_list_storage(self_t&& p_other) noexcept : m_list(std::move(p_other.m_list)), m_map(std::move(p_other.m_map)), m_capacity(p_other.m_capacity) { p_other.m_capacity = 0; }

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_list			   = std::move(p_other.m_list);
						m_map			   = std::move(p_other.m_map);
						m_capacity		   = p_other.m_capacity;
						p_other.m_capacity = 0;
					}
					return *this;
				}

			  public:
				/**
				 * @brief Insert or update a key-value pair at the front of the list
				 * @param p_key Key to insert
				 * @param p_value Value to insert
				 * @return True if a new key was inserted, false if existing key was updated
				 */
				auto insert_front(const key_t& p_key, const value_t& p_value) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Update existing key - remove from current position
						m_list.erase(iter->second.second);
						iter->second.first = p_value;
					}
					else
					{
						// New key - check capacity
						if (m_map.size() >= m_capacity && m_capacity > 0)
						{
							return false; // Caller should handle eviction
						}
					}

					// Insert at front
					m_list.push_front(p_key);
					m_map[p_key] = {p_value, m_list.begin()};
					return iter == m_map.end(); // True if new insertion
				}

				/**
				 * @brief Find a value by key and move it to front
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find_and_move_to_front(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Move to front for LRU semantics
						m_list.splice(m_list.begin(), m_list, iter->second.second);
						return &iter->second.first;
					}
					return nullptr;
				}

				/**
				 * @brief Find a value by key and move it to front (MRU eviction)
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find_and_move_to_front_mru(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Move to front - for MRU, most recently used goes to front (to be evicted first)
						m_list.splice(m_list.begin(), m_list, iter->second.second);
						return &iter->second.first;
					}
					return nullptr;
				}

				/**
				 * @brief Find a value by key without moving it
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? &iter->second.first : nullptr;
				}

				/**
				 * @brief Get the least recently used key (back of list)
				 * @return Key at back of list
				 * @throws std::runtime_error if empty
				 */
				auto get_lru_key() -> key_t
				{
					if (m_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					return m_list.back();
				}

				/**
				 * @brief Get the most recently used key (front of list)
				 * @return Key at front of list
				 * @throws std::runtime_error if empty
				 */
				auto get_mru_key() -> key_t
				{
					if (m_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					return m_list.front();
				}

				/**
				 * @brief Remove a key-value pair
				 * @param p_key Key to remove
				 * @return True if key was found and removed, false otherwise
				 */
				auto erase(const key_t& p_key) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						m_list.erase(iter->second.second);
						m_map.erase(iter);
						return true;
					}
					return false;
				}

				/**
				 * @brief Remove the least recently used item (back of list)
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_lru() -> key_t
				{
					if (m_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					const key_t key = m_list.back();
					m_map.erase(key);
					m_list.pop_back();
					return key;
				}

				/**
				 * @brief Remove the most recently used item (front of list)
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_mru() -> key_t
				{
					if (m_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					const key_t key = m_list.front();
					m_map.erase(key);
					m_list.pop_front();
					return key;
				}

				/**
				 * @brief Get current size
				 * @return Number of stored items
				 */
				auto size() const -> std::size_t { return m_map.size(); }

				/**
				 * @brief Get capacity
				 * @return Maximum number of items
				 */
				auto capacity() const -> std::size_t { return m_capacity; }

				/**
				 * @brief Check if storage is empty
				 * @return True if empty, false otherwise
				 */
				auto empty() const -> bool { return m_map.empty(); }

				/**
				 * @brief Check if storage is full
				 * @return True if at capacity, false otherwise
				 */
				auto full() const -> bool { return m_map.size() >= m_capacity; }

				/**
				 * @brief Clear all items
				 */
				auto clear() -> void
				{
					m_list.clear();
					m_map.clear();
				}
			};
		} // namespace storage
	} // namespace policies
} // namespace cache_engine