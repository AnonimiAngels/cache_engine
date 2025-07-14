// File: inc/cache_engine/policies/storage/hash_queue_storage.hpp

#pragma once

#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace cache_engine
{
	namespace policies
	{
		namespace storage
		{
			/**
			 * @brief Hash queue storage policy optimized for FIFO algorithm
			 * @details Provides O(1) insert and find operations using
			 *          std::queue for FIFO ordering and std::unordered_map for fast lookup
			 * @tparam key_t Key type
			 * @tparam value_t Value type
			 */
			template <typename key_t, typename value_t> class hash_queue_storage
			{
			  public:
				using self_t = hash_queue_storage<key_t, value_t>;

			  private:
				std::unordered_map<key_t, value_t> m_map;
				std::queue<key_t> m_queue;
				std::size_t m_capacity;

			  public:
				~hash_queue_storage() = default;

				explicit hash_queue_storage(std::size_t p_capacity) : m_capacity(p_capacity) {}

				hash_queue_storage(const self_t&)		 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				hash_queue_storage(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_queue(std::move(p_other.m_queue)), m_capacity(p_other.m_capacity)
				{
					p_other.m_capacity = 0;
				}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_map			   = std::move(p_other.m_map);
						m_queue			   = std::move(p_other.m_queue);
						m_capacity		   = p_other.m_capacity;
						p_other.m_capacity = 0;
					}
					return *this;
				}

			  public:
				/**
				 * @brief Insert or update a key-value pair
				 * @param p_key Key to insert
				 * @param p_value Value to insert
				 * @return True if a new key was inserted, false if existing key was updated
				 */
				auto insert(const key_t& p_key, const value_t& p_value) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Update existing key - just update value, don't change queue position
						iter->second = p_value;
						return false;
					}

					// New key - check capacity
					if (m_map.size() >= m_capacity && m_capacity > 0)
					{
						return false; // Caller should handle eviction
					}

					// Insert new key-value pair
					m_map[p_key] = p_value;
					m_queue.push(p_key);
					return true;
				}

				/**
				 * @brief Find a value by key
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? &iter->second : nullptr;
				}

				/**
				 * @brief Find a value by key (const version)
				 * @param p_key Key to find
				 * @return Pointer to const value if found, nullptr otherwise
				 */
				auto find(const key_t& p_key) const -> const value_t*
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? &iter->second : nullptr;
				}

				/**
				 * @brief Get the oldest key (front of queue) - FIFO victim
				 * @return Key at front of queue
				 * @throws std::runtime_error if empty
				 */
				auto get_fifo_key() -> key_t
				{
					if (m_queue.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					return m_queue.front();
				}

				/**
				 * @brief Remove a key-value pair
				 * @param p_key Key to remove
				 * @return True if key was found and removed, false otherwise
				 * @note This operation is O(n) for queue cleanup but rarely used in FIFO
				 */
				auto erase(const key_t& p_key) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						m_map.erase(iter);
						// Note: We don't remove from queue here as it would be O(n)
						// The queue will be cleaned up naturally during FIFO operations
						return true;
					}
					return false;
				}

				/**
				 * @brief Remove the oldest item (FIFO eviction)
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_fifo() -> key_t
				{
					if (m_queue.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					// Handle case where queue may contain keys that were already erased
					key_t key;
					do
					{
						if (m_queue.empty())
						{
							throw std::runtime_error("Storage is empty");
						}
						key = m_queue.front();
						m_queue.pop();
					} while (m_map.find(key) == m_map.end() && !m_queue.empty());

					// If we found a valid key, remove it from map
					if (m_map.find(key) != m_map.end())
					{
						m_map.erase(key);
						return key;
					}

					throw std::runtime_error("Storage is empty");
				}

				/**
				 * @brief Insert or update key-value pair, handling capacity automatically
				 * @param p_key Key to insert
				 * @param p_value Value to insert
				 * @return Key that was evicted (if any), or default-constructed key_t if no eviction
				 */
				auto put(const key_t& p_key, const value_t& p_value) -> key_t
				{
					key_t evicted_key = key_t{};

					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Update existing key
						iter->second = p_value;
						return evicted_key; // No eviction needed
					}

					// New key - check if eviction needed
					if (m_map.size() >= m_capacity && m_capacity > 0)
					{
						evicted_key = erase_fifo();
					}

					// Insert new key-value pair
					m_map[p_key] = p_value;
					m_queue.push(p_key);
					return evicted_key;
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
					m_map.clear();
					// Clear queue by creating a new empty one
					m_queue = std::queue<key_t>{};
				}

				/**
				 * @brief Check if a key exists
				 * @param p_key Key to check
				 * @return True if key exists, false otherwise
				 */
				auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }
			};
		} // namespace storage
	} // namespace policies
} // namespace cache_engine