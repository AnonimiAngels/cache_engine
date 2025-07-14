// File: inc/cache_engine/policies/eviction_policies.hpp

#pragma once

#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "policy_interfaces.hpp"

namespace cache_engine
{
	namespace policies
	{
		/**
		 * @brief Least Recently Used (LRU) eviction policy
		 *
		 * Evicts the least recently used item when cache is full.
		 * Maintains access order using a doubly-linked list.
		 *
		 * Time Complexity:
		 * - on_access: O(1)
		 * - on_insert: O(1)
		 * - select_victim: O(1)
		 * - remove_key: O(1)
		 */
		template <typename key_t, typename value_t> class lru_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = lru_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::list<key_t> m_access_list;
			std::unordered_map<key_t, typename std::list<key_t>::iterator> m_key_to_iterator;

		  public:
			// Constructor
			lru_eviction_policy() = default;

			// Destructor
			~lru_eviction_policy() override = default;

			// Copy constructor and assignment operator (deleted)
			lru_eviction_policy(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			lru_eviction_policy(self_t&& p_other) noexcept : m_access_list(std::move(p_other.m_access_list)), m_key_to_iterator(std::move(p_other.m_key_to_iterator)) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_access_list	  = std::move(p_other.m_access_list);
					m_key_to_iterator = std::move(p_other.m_key_to_iterator);
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override
			{
				auto iter_map = m_key_to_iterator.find(p_key);
				if (iter_map != m_key_to_iterator.end())
				{
					// Move to front (most recently used)
					m_access_list.splice(m_access_list.begin(), m_access_list, iter_map->second);
				}
			}

			auto on_insert(const key_t& p_key) -> void override
			{
				// Add to front (most recently used)
				m_access_list.push_front(p_key);
				m_key_to_iterator[p_key] = m_access_list.begin();
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// Treat update same as access
				this->on_access(p_key);
			}

			auto select_victim() -> key_t override
			{
				if (m_access_list.empty())
				{
					throw std::runtime_error("Cannot select victim from empty LRU policy");
				}

				// Return least recently used (back of list)
				return m_access_list.back();
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto iter_map = m_key_to_iterator.find(p_key);
				if (iter_map != m_key_to_iterator.end())
				{
					m_access_list.erase(iter_map->second);
					m_key_to_iterator.erase(iter_map);
				}
			}

			auto empty() const -> bool override { return m_access_list.empty(); }

			auto size() const -> std::size_t override { return m_access_list.size(); }

			auto clear() -> void override
			{
				m_access_list.clear();
				m_key_to_iterator.clear();
			}
		};

		/**
		 * @brief Most Recently Used (MRU) eviction policy
		 *
		 * Evicts the most recently used item when cache is full.
		 * Maintains access order using a doubly-linked list.
		 *
		 * Time Complexity:
		 * - on_access: O(1)
		 * - on_insert: O(1)
		 * - select_victim: O(1)
		 * - remove_key: O(1)
		 */
		template <typename key_t, typename value_t> class mru_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = mru_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::list<key_t> m_access_list;
			std::unordered_map<key_t, typename std::list<key_t>::iterator> m_key_to_iterator;

		  public:
			// Constructor
			mru_eviction_policy() = default;

			// Destructor
			~mru_eviction_policy() override = default;

			// Copy constructor and assignment operator (deleted)
			mru_eviction_policy(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			mru_eviction_policy(self_t&& p_other) noexcept : m_access_list(std::move(p_other.m_access_list)), m_key_to_iterator(std::move(p_other.m_key_to_iterator)) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_access_list	  = std::move(p_other.m_access_list);
					m_key_to_iterator = std::move(p_other.m_key_to_iterator);
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override
			{
				auto iter_map = m_key_to_iterator.find(p_key);
				if (iter_map != m_key_to_iterator.end())
				{
					// Move to front (most recently used)
					m_access_list.splice(m_access_list.begin(), m_access_list, iter_map->second);
				}
			}

			auto on_insert(const key_t& p_key) -> void override
			{
				// Add to front (most recently used)
				m_access_list.push_front(p_key);
				m_key_to_iterator[p_key] = m_access_list.begin();
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// Treat update same as access
				this->on_access(p_key);
			}

			auto select_victim() -> key_t override
			{
				if (m_access_list.empty())
				{
					throw std::runtime_error("Cannot select victim from empty MRU policy");
				}

				// Return most recently used (front of list)
				return m_access_list.front();
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto iter_map = m_key_to_iterator.find(p_key);
				if (iter_map != m_key_to_iterator.end())
				{
					m_access_list.erase(iter_map->second);
					m_key_to_iterator.erase(iter_map);
				}
			}

			auto empty() const -> bool override { return m_access_list.empty(); }

			auto size() const -> std::size_t override { return m_access_list.size(); }

			auto clear() -> void override
			{
				m_access_list.clear();
				m_key_to_iterator.clear();
			}
		};

		/**
		 * @brief First In First Out (FIFO) eviction policy
		 *
		 * Evicts the oldest inserted item when cache is full.
		 * Maintains insertion order using a queue.
		 *
		 * Time Complexity:
		 * - on_access: O(1) (no-op)
		 * - on_insert: O(1)
		 * - select_victim: O(1)
		 * - remove_key: O(n) worst case
		 */
		template <typename key_t, typename value_t> class fifo_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = fifo_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::queue<key_t> m_insertion_queue;
			std::unordered_map<key_t, bool> m_key_exists;

		  public:
			// Constructor
			fifo_eviction_policy() = default;

			// Destructor
			~fifo_eviction_policy() override = default;

			// Copy constructor and assignment operator (deleted)
			fifo_eviction_policy(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			fifo_eviction_policy(self_t&& p_other) noexcept : m_insertion_queue(std::move(p_other.m_insertion_queue)), m_key_exists(std::move(p_other.m_key_exists)) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_insertion_queue = std::move(p_other.m_insertion_queue);
					m_key_exists	  = std::move(p_other.m_key_exists);
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override
			{
				// FIFO doesn't care about access patterns
				static_cast<void>(p_key);
			}

			auto on_insert(const key_t& p_key) -> void override
			{
				m_insertion_queue.push(p_key);
				m_key_exists[p_key] = true;
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// FIFO doesn't care about updates
				static_cast<void>(p_key);
			}

			auto select_victim() -> key_t override
			{
				// Clean up any keys that were removed externally
				while (!m_insertion_queue.empty())
				{
					const key_t candidate = m_insertion_queue.front();
					m_insertion_queue.pop();

					auto iter = m_key_exists.find(candidate);
					if (iter != m_key_exists.end() && iter->second)
					{
						return candidate;
					}
				}

				throw std::runtime_error("Cannot select victim from empty FIFO policy");
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto iter = m_key_exists.find(p_key);
				if (iter != m_key_exists.end())
				{
					iter->second = false; // Mark as removed
				}
			}

			auto empty() const -> bool override { return m_key_exists.empty(); }

			auto size() const -> std::size_t override
			{
				// Count only existing keys
				std::size_t count = 0;
				for (const auto& pair : m_key_exists)
				{
					if (pair.second)
					{
						++count;
					}
				}
				return count;
			}

			auto clear() -> void override
			{
				// Clear queue by creating new empty queue
				m_insertion_queue = std::queue<key_t>();
				m_key_exists.clear();
			}
		};

		/**
		 * @brief Least Frequently Used (LFU) eviction policy
		 *
		 * Evicts the least frequently accessed item when cache is full.
		 * Maintains frequency counters and frequency buckets.
		 *
		 * Time Complexity:
		 * - on_access: O(log F) where F is number of unique frequencies
		 * - on_insert: O(log F)
		 * - select_victim: O(log F)
		 * - remove_key: O(log F)
		 */
		template <typename key_t, typename value_t> class lfu_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = lfu_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, std::size_t> m_key_frequency;
			std::map<std::size_t, std::list<key_t>> m_frequency_buckets;
			std::unordered_map<key_t, typename std::list<key_t>::iterator> m_key_to_iterator;

		  public:
			// Constructor
			lfu_eviction_policy() = default;

			// Destructor
			~lfu_eviction_policy() override = default;

			// Move constructor and assignment operator
			lfu_eviction_policy(self_t&& p_other) noexcept
				: m_key_frequency(std::move(p_other.m_key_frequency)), m_frequency_buckets(std::move(p_other.m_frequency_buckets)), m_key_to_iterator(std::move(p_other.m_key_to_iterator))
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_key_frequency		= std::move(p_other.m_key_frequency);
					m_frequency_buckets = std::move(p_other.m_frequency_buckets);
					m_key_to_iterator	= std::move(p_other.m_key_to_iterator);
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override { this->increment_frequency(p_key); }

			auto on_insert(const key_t& p_key) -> void override
			{
				// Start with frequency 1
				m_key_frequency[p_key] = 1;
				m_frequency_buckets[1].push_back(p_key);
				m_key_to_iterator[p_key] = std::prev(m_frequency_buckets[1].end());
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// Treat update same as access
				this->increment_frequency(p_key);
			}

			auto select_victim() -> key_t override
			{
				if (m_frequency_buckets.empty())
				{
					throw std::runtime_error("Cannot select victim from empty LFU policy");
				}

				// Find the lowest frequency bucket with items
				auto lowest_freq_iter = m_frequency_buckets.begin();
				while (lowest_freq_iter != m_frequency_buckets.end() && lowest_freq_iter->second.empty())
				{
					++lowest_freq_iter;
				}

				if (lowest_freq_iter == m_frequency_buckets.end())
				{
					throw std::runtime_error("Cannot select victim from empty LFU policy");
				}

				// Return first item in lowest frequency bucket
				return lowest_freq_iter->second.front();
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto freq_iter = m_key_frequency.find(p_key);
				if (freq_iter != m_key_frequency.end())
				{
					const std::size_t frequency = freq_iter->second;

					// Remove from frequency bucket
					auto bucket_iter = m_frequency_buckets.find(frequency);
					if (bucket_iter != m_frequency_buckets.end())
					{
						auto key_iter = m_key_to_iterator.find(p_key);
						if (key_iter != m_key_to_iterator.end())
						{
							bucket_iter->second.erase(key_iter->second);

							// Clean up empty bucket
							if (bucket_iter->second.empty())
							{
								m_frequency_buckets.erase(bucket_iter);
							}

							m_key_to_iterator.erase(key_iter);
						}
					}

					m_key_frequency.erase(freq_iter);
				}
			}

			auto empty() const -> bool override { return m_key_frequency.empty(); }

			auto size() const -> std::size_t override { return m_key_frequency.size(); }

			auto clear() -> void override
			{
				m_key_frequency.clear();
				m_frequency_buckets.clear();
				m_key_to_iterator.clear();
			}

		  private:
			auto increment_frequency(const key_t& p_key) -> void
			{
				auto freq_iter = m_key_frequency.find(p_key);
				if (freq_iter != m_key_frequency.end())
				{
					const std::size_t old_frequency = freq_iter->second;
					const std::size_t new_frequency = old_frequency + 1;

					// Remove from old frequency bucket
					auto old_bucket_iter = m_frequency_buckets.find(old_frequency);
					if (old_bucket_iter != m_frequency_buckets.end())
					{
						auto key_iter = m_key_to_iterator.find(p_key);
						if (key_iter != m_key_to_iterator.end())
						{
							old_bucket_iter->second.erase(key_iter->second);

							// Clean up empty bucket
							if (old_bucket_iter->second.empty())
							{
								m_frequency_buckets.erase(old_bucket_iter);
							}
						}
					}

					// Add to new frequency bucket
					freq_iter->second = new_frequency;
					m_frequency_buckets[new_frequency].push_back(p_key);
					m_key_to_iterator[p_key] = std::prev(m_frequency_buckets[new_frequency].end());
				}
			}
		};

		/**
		 * @brief Most Frequently Used (MFU) eviction policy
		 *
		 * Evicts the most frequently accessed item when cache is full.
		 * Maintains frequency counters and frequency buckets.
		 *
		 * Time Complexity:
		 * - on_access: O(log F) where F is number of unique frequencies
		 * - on_insert: O(log F)
		 * - select_victim: O(log F)
		 * - remove_key: O(log F)
		 */
		template <typename key_t, typename value_t> class mfu_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = mfu_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, std::size_t> m_key_frequency;
			std::map<std::size_t, std::list<key_t>> m_frequency_buckets;
			std::unordered_map<key_t, typename std::list<key_t>::iterator> m_key_to_iterator;

		  public:
			// Constructor
			mfu_eviction_policy() = default;

			// Destructor
			~mfu_eviction_policy() override = default;

			// Move constructor and assignment operator
			mfu_eviction_policy(self_t&& p_other) noexcept
				: m_key_frequency(std::move(p_other.m_key_frequency)), m_frequency_buckets(std::move(p_other.m_frequency_buckets)), m_key_to_iterator(std::move(p_other.m_key_to_iterator))
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_key_frequency		= std::move(p_other.m_key_frequency);
					m_frequency_buckets = std::move(p_other.m_frequency_buckets);
					m_key_to_iterator	= std::move(p_other.m_key_to_iterator);
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override { this->increment_frequency(p_key); }

			auto on_insert(const key_t& p_key) -> void override
			{
				// Start with frequency 1
				m_key_frequency[p_key] = 1;
				m_frequency_buckets[1].push_back(p_key);
				m_key_to_iterator[p_key] = std::prev(m_frequency_buckets[1].end());
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// Treat update same as access
				this->increment_frequency(p_key);
			}

			auto select_victim() -> key_t override
			{
				if (m_frequency_buckets.empty())
				{
					throw std::runtime_error("Cannot select victim from empty MFU policy");
				}

				// Find the highest frequency bucket with items
				auto highest_freq_iter = m_frequency_buckets.rbegin();
				while (highest_freq_iter != m_frequency_buckets.rend() && highest_freq_iter->second.empty())
				{
					++highest_freq_iter;
				}

				if (highest_freq_iter == m_frequency_buckets.rend())
				{
					throw std::runtime_error("Cannot select victim from empty MFU policy");
				}

				// Return first item in highest frequency bucket
				return highest_freq_iter->second.front();
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto freq_iter = m_key_frequency.find(p_key);
				if (freq_iter != m_key_frequency.end())
				{
					const std::size_t frequency = freq_iter->second;

					// Remove from frequency bucket
					auto bucket_iter = m_frequency_buckets.find(frequency);
					if (bucket_iter != m_frequency_buckets.end())
					{
						auto key_iter = m_key_to_iterator.find(p_key);
						if (key_iter != m_key_to_iterator.end())
						{
							bucket_iter->second.erase(key_iter->second);

							// Clean up empty bucket
							if (bucket_iter->second.empty())
							{
								m_frequency_buckets.erase(bucket_iter);
							}

							m_key_to_iterator.erase(key_iter);
						}
					}

					m_key_frequency.erase(freq_iter);
				}
			}

			auto empty() const -> bool override { return m_key_frequency.empty(); }

			auto size() const -> std::size_t override { return m_key_frequency.size(); }

			auto clear() -> void override
			{
				m_key_frequency.clear();
				m_frequency_buckets.clear();
				m_key_to_iterator.clear();
			}

		  private:
			auto increment_frequency(const key_t& p_key) -> void
			{
				auto freq_iter = m_key_frequency.find(p_key);
				if (freq_iter != m_key_frequency.end())
				{
					const std::size_t old_frequency = freq_iter->second;
					const std::size_t new_frequency = old_frequency + 1;

					// Remove from old frequency bucket
					auto old_bucket_iter = m_frequency_buckets.find(old_frequency);
					if (old_bucket_iter != m_frequency_buckets.end())
					{
						auto key_iter = m_key_to_iterator.find(p_key);
						if (key_iter != m_key_to_iterator.end())
						{
							old_bucket_iter->second.erase(key_iter->second);

							// Clean up empty bucket
							if (old_bucket_iter->second.empty())
							{
								m_frequency_buckets.erase(old_bucket_iter);
							}
						}
					}

					// Add to new frequency bucket
					freq_iter->second = new_frequency;
					m_frequency_buckets[new_frequency].push_back(p_key);
					m_key_to_iterator[p_key] = std::prev(m_frequency_buckets[new_frequency].end());
				}
			}
		};

		/**
		 * @brief Random eviction policy
		 *
		 * Evicts a randomly selected item when cache is full.
		 * Uses optimized O(1) random selection with swap-and-pop.
		 *
		 * Time Complexity:
		 * - on_access: O(1) (no-op)
		 * - on_insert: O(1)
		 * - select_victim: O(1)
		 * - remove_key: O(1) amortized
		 */
		template <typename key_t, typename value_t> class random_eviction_policy : public eviction_policy_base<key_t, value_t>
		{
		  public:
			using self_t = random_eviction_policy<key_t, value_t>;
			using base_t = eviction_policy_base<key_t, value_t>;

		  private:
			std::vector<key_t> m_keys;
			std::unordered_map<key_t, std::size_t> m_key_to_index;
			bool m_random_initialized;

		  public:
			// Constructor
			random_eviction_policy() : m_random_initialized(false) { this->initialize_random(); }

			// Destructor
			~random_eviction_policy() override = default;

			// Move constructor and assignment operator
			random_eviction_policy(self_t&& p_other) noexcept
				: m_keys(std::move(p_other.m_keys)), m_key_to_index(std::move(p_other.m_key_to_index)), m_random_initialized(p_other.m_random_initialized)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_keys				 = std::move(p_other.m_keys);
					m_key_to_index		 = std::move(p_other.m_key_to_index);
					m_random_initialized = p_other.m_random_initialized;
				}
				return *this;
			}

		  public:
			auto on_access(const key_t& p_key) -> void override
			{
				// Random policy doesn't care about access patterns
				static_cast<void>(p_key);
			}

			auto on_insert(const key_t& p_key) -> void override
			{
				const std::size_t new_index = m_keys.size();
				m_keys.push_back(p_key);
				m_key_to_index[p_key] = new_index;
			}

			auto on_update(const key_t& p_key) -> void override
			{
				// Random policy doesn't care about updates
				static_cast<void>(p_key);
			}

			auto select_victim() -> key_t override
			{
				if (m_keys.empty())
				{
					throw std::runtime_error("Cannot select victim from empty random policy");
				}

				// Select random index
				const std::size_t random_index = static_cast<std::size_t>(std::rand()) % m_keys.size();
				return m_keys[random_index];
			}

			auto remove_key(const key_t& p_key) -> void override
			{
				auto index_iter = m_key_to_index.find(p_key);
				if (index_iter != m_key_to_index.end())
				{
					const std::size_t key_index	 = index_iter->second;
					const std::size_t last_index = m_keys.size() - 1;

					if (key_index != last_index)
					{
						// Swap with last element
						const key_t last_key	 = m_keys[last_index];
						m_keys[key_index]		 = last_key;
						m_key_to_index[last_key] = key_index;
					}

					// Remove last element
					m_keys.pop_back();
					m_key_to_index.erase(index_iter);
				}
			}

			auto empty() const -> bool override { return m_keys.empty(); }

			auto size() const -> std::size_t override { return m_keys.size(); }

			auto clear() -> void override
			{
				m_keys.clear();
				m_key_to_index.clear();
			}

		  private:
			auto initialize_random() -> void
			{
				if (!m_random_initialized)
				{
					std::srand(static_cast<unsigned int>(std::time(nullptr)));
					m_random_initialized = true;
				}
			}
		};

	} // namespace policies
} // namespace cache_engine