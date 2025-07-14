// File: inc/cache_engine/policies/storage/optimized_random_storage.hpp

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace cache_engine
{
	namespace policies
	{
		namespace storage
		{
			/**
			 * @brief Optimized random storage policy with O(1) operations
			 * @details Fixes the O(n) vector::erase issue using swap-and-pop technique
			 *          Uses std::unordered_map for values and indices, std::vector for O(1) random access
			 * @tparam key_t Key type
			 * @tparam value_t Value type
			 */
			template <typename key_t, typename value_t> class optimized_random_storage
			{
			  public:
				using self_t			 = optimized_random_storage<key_t, value_t>;
				using index_t			 = std::size_t;
				using value_index_pair_t = std::pair<value_t, index_t>;

			  private:
				std::unordered_map<key_t, value_index_pair_t> m_map; // key -> (value, index in vector)
				std::vector<key_t> m_keys;							 // parallel array for O(1) random access
				std::size_t m_capacity;
				mutable std::mt19937 m_rng; // Random number generator

			  public:
				~optimized_random_storage() = default;

				explicit optimized_random_storage(std::size_t p_capacity) : m_capacity(p_capacity), m_rng(static_cast<std::uint32_t>(std::time(nullptr)))
				{
					m_keys.reserve(p_capacity); // Pre-allocate for better performance
				}

				optimized_random_storage(const self_t&)	 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				optimized_random_storage(self_t&& p_other) noexcept
					: m_map(std::move(p_other.m_map)), m_keys(std::move(p_other.m_keys)), m_capacity(p_other.m_capacity), m_rng(std::move(p_other.m_rng))
				{
					p_other.m_capacity = 0;
				}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_map			   = std::move(p_other.m_map);
						m_keys			   = std::move(p_other.m_keys);
						m_capacity		   = p_other.m_capacity;
						m_rng			   = std::move(p_other.m_rng);
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
						// Update existing key - just update value, don't change position
						iter->second.first = p_value;
						return false;
					}

					// New key - check capacity
					if (m_map.size() >= m_capacity && m_capacity > 0)
					{
						return false; // Caller should handle eviction
					}

					// Insert new key-value pair
					const index_t new_index = m_keys.size();
					m_keys.push_back(p_key);
					m_map[p_key] = {p_value, new_index};
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
				 * @brief Get a random key for eviction
				 * @return Random key from storage
				 * @throws std::runtime_error if empty
				 */
				auto get_random_key() -> key_t
				{
					if (m_keys.empty())
					{
						throw std::runtime_error("Storage is empty");
					}
					const index_t random_index = generate_random_index();
					return m_keys[random_index];
				}

				/**
				 * @brief Remove a key-value pair using O(1) swap-and-pop
				 * @param p_key Key to remove
				 * @return True if key was found and removed, false otherwise
				 */
				auto erase(const key_t& p_key) -> bool
				{
					auto iter = m_map.find(p_key);
					if (iter != m_map.end())
					{
						const index_t remove_index = iter->second.second;

						// Swap-and-pop technique for O(1) removal
						if (remove_index != m_keys.size() - 1)
						{
							// Swap with last element
							const key_t& last_key = m_keys.back();
							std::swap(m_keys[remove_index], m_keys.back());

							// Update the index of the swapped element
							m_map[last_key].second = remove_index;
						}

						// Remove last element (O(1))
						m_keys.pop_back();

						// Remove from map
						m_map.erase(iter);
						return true;
					}
					return false;
				}

				/**
				 * @brief Remove a random item (random eviction)
				 * @return Key that was removed
				 * @throws std::runtime_error if empty
				 */
				auto erase_random() -> key_t
				{
					if (m_keys.empty())
					{
						throw std::runtime_error("Storage is empty");
					}

					const index_t random_index = generate_random_index();
					const key_t victim_key	   = m_keys[random_index];

					// Use swap-and-pop for O(1) removal
					if (random_index != m_keys.size() - 1)
					{
						// Swap with last element
						const key_t& last_key = m_keys.back();
						std::swap(m_keys[random_index], m_keys.back());

						// Update the index of the swapped element
						m_map[last_key].second = random_index;
					}

					// Remove last element (O(1))
					m_keys.pop_back();

					// Remove from map
					m_map.erase(victim_key);
					return victim_key;
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
						iter->second.first = p_value;
						return evicted_key; // No eviction needed
					}

					// New key - check if eviction needed
					if (m_map.size() >= m_capacity && m_capacity > 0)
					{
						evicted_key = erase_random();
					}

					// Insert new key-value pair
					const index_t new_index = m_keys.size();
					m_keys.push_back(p_key);
					m_map[p_key] = {p_value, new_index};
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
					m_keys.clear();
				}

				/**
				 * @brief Check if a key exists
				 * @param p_key Key to check
				 * @return True if key exists, false otherwise
				 */
				auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

				/**
				 * @brief Set random seed for testing purposes
				 * @param p_seed Seed value
				 */
				auto set_seed(std::uint32_t p_seed) -> void { m_rng.seed(p_seed); }

				/**
				 * @brief Get all keys (mainly for testing/debugging)
				 * @return Vector of all keys
				 */
				auto get_all_keys() const -> std::vector<key_t> { return m_keys; }

			  private:
				/**
				 * @brief Generate a random index within the current size
				 * @return Random index [0, size-1]
				 */
				auto generate_random_index() const -> index_t
				{
					if (m_keys.empty())
					{
						throw std::runtime_error("Cannot generate random index for empty storage");
					}

					std::uniform_int_distribution<index_t> dist(0, m_keys.size() - 1);
					return dist(m_rng);
				}

				/**
				 * @brief Validate internal consistency (debug helper)
				 * @return True if consistent, false otherwise
				 */
				auto validate_consistency() const -> bool
				{
					// Check that map size equals vector size
					if (m_map.size() != m_keys.size())
					{
						return false;
					}

					// Check that every key in vector exists in map with correct index
					for (index_t idx = 0; idx < m_keys.size(); ++idx)
					{
						const key_t& key = m_keys[idx];
						auto iter		 = m_map.find(key);
						if (iter == m_map.end() || iter->second.second != idx)
						{
							return false;
						}
					}

					// Check that every key in map exists in vector at correct index
					for (const auto& pair : m_map)
					{
						const key_t& key	= pair.first;
						const index_t index = pair.second.second;
						if (index >= m_keys.size() || m_keys[index] != key)
						{
							return false;
						}
					}

					return true;
				}
			};
		} // namespace storage
	} // namespace policies
} // namespace cache_engine