// File: inc/cache_engine/policies/storage/frequency_map_storage.hpp

#pragma once

#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace cache_engine
{
	namespace policies
	{
		namespace storage
		{
			/**
			 * @brief Frequency map storage policy optimized for LFU/MFU algorithms
			 * @details Provides O(log F) operations using frequency buckets where F is the number of unique frequencies
			 *          Uses std::map for frequency ordering and std::list for items at each frequency
			 * @tparam key_t Key type
			 * @tparam value_t Value type
			 */
			template <typename key_t, typename value_t> class frequency_map_storage
			{
			  public:
				using self_t					 = frequency_map_storage<key_t, value_t>;
				using frequency_t				 = std::size_t;
				using key_list_t				 = std::list<key_t>;
				using frequency_map_t			 = std::map<frequency_t, key_list_t>;
				using list_iterator_t			 = typename key_list_t::iterator;
				using value_frequency_iterator_t = std::pair<value_t, std::pair<frequency_t, list_iterator_t>>;

			  private:
				std::unordered_map<key_t, value_frequency_iterator_t> m_map;
				frequency_map_t m_freq_map;
				std::size_t m_capacity;

			  public:
				~frequency_map_storage() = default;

				explicit frequency_map_storage(std::size_t p_capacity) : m_capacity(p_capacity) {}

				frequency_map_storage(const self_t&)	 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				frequency_map_storage(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_freq_map(std::move(p_other.m_freq_map)), m_capacity(p_other.m_capacity)
				{
					p_other.m_capacity = 0;
				}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_map			   = std::move(p_other.m_map);
						m_freq_map		   = std::move(p_other.m_freq_map);
						m_capacity		   = p_other.m_capacity;
						p_other.m_capacity = 0;
					}
					return *this;
				}

			  public:
				/**
				 * @brief Insert or update a key-value pair with frequency 1
				 * @param p_key Key to insert
				 * @param p_value Value to insert
				 * @return True if a new key was inserted, false if existing key was updated
				 */
				auto insert(const key_t& p_key, const value_t& p_value) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						// Update existing key - increment frequency
						iter->second.first = p_value;
						increment_frequency(p_key);
						return false;
					}

					// New key - check capacity
					if (m_map.size() >= m_capacity && m_capacity > 0)
					{
						return false; // Caller should handle eviction
					}

					// Insert new key with frequency 1
					m_freq_map[1].push_back(p_key);
					auto list_iter = std::prev(m_freq_map[1].end());
					m_map[p_key]   = {p_value, {1, list_iter}};
					return true;
				}

				/**
				 * @brief Find a value by key and increment its frequency
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find_and_increment(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						increment_frequency(p_key);
						return &iter->second.first;
					}
					return nullptr;
				}

				/**
				 * @brief Find a value by key without incrementing frequency
				 * @param p_key Key to find
				 * @return Pointer to value if found, nullptr otherwise
				 */
				auto find(const key_t& p_key) -> value_t*
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? &iter->second.first : nullptr;
				}

				/**
				 * @brief Find a value by key (const version)
				 * @param p_key Key to find
				 * @return Pointer to const value if found, nullptr otherwise
				 */
				auto find(const key_t& p_key) const -> const value_t*
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? &iter->second.first : nullptr;
				}

				/**
				 * @brief Get the least frequently used key
				 * @return Key with lowest frequency (first in lowest frequency bucket)
				 * @throws std::runtime_error if empty
				 */
				auto get_lfu_key() -> key_t
				{
					if (m_freq_map.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					auto& least_freq_list = m_freq_map.begin()->second;
					if (least_freq_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					return least_freq_list.front();
				}

				/**
				 * @brief Get the most frequently used key
				 * @return Key with highest frequency (first in highest frequency bucket)
				 * @throws std::runtime_error if empty
				 */
				auto get_mfu_key() -> key_t
				{
					if (m_freq_map.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					auto& most_freq_list = m_freq_map.rbegin()->second;
					if (most_freq_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					return most_freq_list.front();
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
						const frequency_t freq = iter->second.second.first;
						auto list_iter		   = iter->second.second.second;

						// Remove from frequency list
						m_freq_map[freq].erase(list_iter);
						if (m_freq_map[freq].empty())
						{
							m_freq_map.erase(freq);
						}

						// Remove from main map
						m_map.erase(iter);
						return true;
					}
					return false;
				}

				/**
				 * @brief Remove the least frequently used item
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_lfu() -> key_t
				{
					if (m_freq_map.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					auto& least_freq_list = m_freq_map.begin()->second;
					if (least_freq_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					const key_t key = least_freq_list.front();
					least_freq_list.pop_front();

					if (least_freq_list.empty())
					{
						m_freq_map.erase(m_freq_map.begin());
					}

					m_map.erase(key);
					return key;
				}

				/**
				 * @brief Remove the most frequently used item
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_mfu() -> key_t
				{
					if (m_freq_map.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					auto& most_freq_list = m_freq_map.rbegin()->second;
					if (most_freq_list.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					const key_t key = most_freq_list.front();
					most_freq_list.pop_front();

					if (most_freq_list.empty())
					{
						m_freq_map.erase(std::prev(m_freq_map.end()));
					}

					m_map.erase(key);
					return key;
				}

				/**
				 * @brief Get the frequency of a key
				 * @param p_key Key to check
				 * @return Frequency of the key, 0 if key not found
				 */
				auto get_frequency(const key_t& p_key) const -> frequency_t
				{
					auto iter = m_map.find(p_key);
					return iter != m_map.end() ? iter->second.second.first : 0;
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
					m_freq_map.clear();
				}

				/**
				 * @brief Check if a key exists
				 * @param p_key Key to check
				 * @return True if key exists, false otherwise
				 */
				auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

				/**
				 * @brief Get number of unique frequencies
				 * @return Number of different frequency buckets
				 */
				auto frequency_count() const -> std::size_t { return m_freq_map.size(); }

			  private:
				/**
				 * @brief Increment frequency of a key
				 * @param p_key Key to increment frequency for
				 */
				auto increment_frequency(const key_t& p_key) -> void
				{
					auto iter = m_map.find(p_key);
					if (iter == m_map.end())
					{
						return;
					}

					auto& freq		= iter->second.second.first;
					auto& list_iter = iter->second.second.second;

					// Remove from current frequency bucket
					m_freq_map[freq].erase(list_iter);
					if (m_freq_map[freq].empty())
					{
						m_freq_map.erase(freq);
					}

					// Increment frequency and add to new bucket
					freq++;
					m_freq_map[freq].push_back(p_key);
					list_iter = std::prev(m_freq_map[freq].end());
				}
			};
		} // namespace storage
	} // namespace policies
} // namespace cache_engine