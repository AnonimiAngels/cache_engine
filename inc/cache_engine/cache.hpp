#pragma once

/**
 * @file cache_new.hpp
 * @brief Modern policy-based cache implementation
 *
 * This header provides the new policy-based cache interface alongside
 * backward compatibility with the original template specialization approach.
 *
 * Usage Examples:
 *
 * // Backward compatible (existing code works unchanged):
 * cache_engine::cache<int, std::string, cache_engine::algorithm::lru> old_cache(100);
 *
 * // New policy-based approach (recommended):
 * cache_engine::policy_based_cache<
 *     int, std::string,
 *     cache_engine::policy_templates::lru_eviction,
 *     cache_engine::policy_templates::hash_storage,
 *     cache_engine::policy_templates::update_on_access,
 *     cache_engine::policy_templates::fixed_capacity
 * > new_cache(100);
 *
 * // Using predefined policy sets:
 * auto lru_cache = cache_engine::make_cache<cache_engine::policies::lru_policy_set>(100);
 * auto adaptive_cache = cache_engine::make_cache<cache_engine::policies::adaptive_policy_set>(100);
 */

// Core includes for both template specialization and policy-based implementations
#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "policies/policy_interfaces.hpp"
#include "policies/policy_traits.hpp"
#include "policies/all_policies.hpp"

namespace cache_engine
{
	// Constants for adaptive cache defaults
	constexpr std::size_t default_min_capacity = 32U;
	constexpr std::size_t default_max_capacity = 4096U;

	namespace algorithm
	{
		class lru;			// Least Recently Used
		class lfu;			// Least Frequently Used
		class mfu;			// Most Frequently Used
		class mru;			// Most Recently Used
		class fifo;			// First In First Out
		class random_cache; // Random Replacement
	} // namespace algorithm

	template <typename key_t, typename value_t, typename algorithm_t = algorithm::lru> class cache;

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::lru>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::lru>;

	  private:
		std::list<key_t> m_list;
		std::unordered_map<key_t, std::pair<value_t, typename std::list<key_t>::iterator>> m_map;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) {}

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_list(std::move(p_other.m_list)), m_map(std::move(p_other.m_map)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_list	   = std::move(p_other.m_list);
				m_map	   = std::move(p_other.m_map);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			if (m_map.find(p_key) != m_map.end())
			{
				m_list.erase(m_map[p_key].second);
			}
			else if (m_map.size() >= m_capacity)
			{
				m_map.erase(m_list.back());
				m_list.pop_back();
			}
			m_list.push_front(p_key);
			m_map[p_key] = {p_value, m_list.begin()};
		}

		auto get(const key_t& p_key) -> value_t
		{
			auto iter = m_map.find(p_key);
			if (iter != m_map.end())
			{
				m_list.splice(m_list.begin(), m_list, iter->second.second);
				return iter->second.first;
			}
			throw std::out_of_range("Key not found");
		}

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			m_list.clear();
		}
	};

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::fifo>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::fifo>;

	  private:
		std::unordered_map<key_t, value_t> m_map;
		std::queue<key_t> m_queue;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) {}

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_queue(std::move(p_other.m_queue)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_map	   = std::move(p_other.m_map);
				m_queue	   = std::move(p_other.m_queue);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			if (m_map.find(p_key) != m_map.end())
			{
				m_map[p_key] = p_value;
				return;
			}

			if (m_map.size() >= m_capacity)
			{
				m_map.erase(m_queue.front());
				m_queue.pop();
			}

			m_map[p_key] = p_value;
			m_queue.push(p_key);
		}

		auto get(const key_t& p_key) -> value_t { return m_map.at(p_key); }

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			while (!m_queue.empty())
			{
				m_queue.pop();
			}
		}
	};

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::lfu>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::lfu>;

	  private:
		std::unordered_map<key_t, std::pair<value_t, std::size_t>> m_map;
		std::map<std::size_t, std::list<key_t>> m_freq_map;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) {}

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_freq_map(std::move(p_other.m_freq_map)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_map	   = std::move(p_other.m_map);
				m_freq_map = std::move(p_other.m_freq_map);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			if (m_capacity == 0)
			{
				return;
			}

			if (m_map.find(p_key) != m_map.end())
			{
				m_map[p_key].first = p_value;

				auto& freq = m_map[p_key].second;
				m_freq_map[freq].remove(p_key);
				if (m_freq_map[freq].empty())
				{
					m_freq_map.erase(freq);
				}
				freq++;
				m_freq_map[freq].push_back(p_key);

				return;
			}
			if (m_map.size() >= m_capacity)
			{
				auto& least_freq_list = m_freq_map.begin()->second;
				m_map.erase(least_freq_list.front());
				least_freq_list.pop_front();
				if (least_freq_list.empty())
				{
					m_freq_map.erase(m_freq_map.begin());
				}
			}
			m_map[p_key] = {p_value, 1};
			m_freq_map[1].push_back(p_key);
		}

		auto get(const key_t& p_key) -> value_t
		{
			auto iter = m_map.find(p_key);
			if (iter == m_map.end())
			{
				throw std::out_of_range("Key not found");
			}

			auto& freq = iter->second.second;
			auto& val  = iter->second.first;

			m_freq_map[freq].remove(p_key);
			if (m_freq_map[freq].empty())
			{
				m_freq_map.erase(freq);
			}

			freq++;
			m_freq_map[freq].push_back(p_key);
			return val;
		}

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			m_freq_map.clear();
		}
	};

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::mfu>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::mfu>;

	  private:
		std::unordered_map<key_t, std::pair<value_t, std::size_t>> m_map;
		std::map<std::size_t, std::list<key_t>> m_freq_map;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) {}

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_freq_map(std::move(p_other.m_freq_map)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_map	   = std::move(p_other.m_map);
				m_freq_map = std::move(p_other.m_freq_map);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			if (m_capacity == 0)
			{
				return;
			}

			if (m_map.find(p_key) != m_map.end())
			{
				m_map[p_key].first = p_value;

				auto& freq = m_map[p_key].second;
				m_freq_map[freq].remove(p_key);
				if (m_freq_map[freq].empty())
				{
					m_freq_map.erase(freq);
				}
				freq++;
				m_freq_map[freq].push_back(p_key);

				return;
			}
			if (m_map.size() >= m_capacity)
			{
				auto& most_freq_list = m_freq_map.rbegin()->second;
				m_map.erase(most_freq_list.front());
				most_freq_list.pop_front();
				if (most_freq_list.empty())
				{
					m_freq_map.erase(std::prev(m_freq_map.end()));
				}
			}
			m_map[p_key] = {p_value, 1};
			m_freq_map[1].push_back(p_key);
		}

		auto get(const key_t& p_key) -> value_t
		{
			auto iter = m_map.find(p_key);
			if (iter == m_map.end())
			{
				throw std::out_of_range("Key not found");
			}

			auto& freq = iter->second.second;
			auto& val  = iter->second.first;

			m_freq_map[freq].remove(p_key);
			if (m_freq_map[freq].empty())
			{
				m_freq_map.erase(freq);
			}
			freq++;
			m_freq_map[freq].push_back(p_key);
			return val;
		}

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			m_freq_map.clear();
		}
	};

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::mru>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::mru>;

	  private:
		std::list<key_t> m_list;
		std::unordered_map<key_t, std::pair<value_t, typename std::list<key_t>::iterator>> m_map;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) {}

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_list(std::move(p_other.m_list)), m_map(std::move(p_other.m_map)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_list	   = std::move(p_other.m_list);
				m_map	   = std::move(p_other.m_map);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			if (m_map.find(p_key) != m_map.end())
			{
				m_list.erase(m_map[p_key].second);
			}
			else if (m_map.size() >= m_capacity)
			{
				m_map.erase(m_list.front());
				m_list.pop_front();
			}
			m_list.push_front(p_key);
			m_map[p_key] = {p_value, m_list.begin()};
		}

		auto get(const key_t& p_key) -> value_t
		{
			auto iter = m_map.find(p_key);
			if (iter != m_map.end())
			{
				m_list.splice(m_list.begin(), m_list, iter->second.second);
				return iter->second.first;
			}
			throw std::out_of_range("Key not found");
		}

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			m_list.clear();
		}
	};

	template <typename key_t, typename value_t> class cache<key_t, value_t, algorithm::random_cache>
	{
	  public:
		using self_t = cache<key_t, value_t, algorithm::random_cache>;

	  private:
		std::unordered_map<key_t, std::pair<value_t, std::size_t>> m_map;
		std::vector<key_t> m_keys;
		std::size_t m_capacity;

	  public:
		explicit cache(std::size_t p_capacity) : m_capacity(p_capacity) { std::srand(static_cast<unsigned int>(std::time(nullptr))); }

		// Destructor
		~cache() {}

		// Deleted copy constructor and assignment operator
		cache(const self_t&)					 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		cache(self_t&& p_other) noexcept : m_map(std::move(p_other.m_map)), m_keys(std::move(p_other.m_keys)), m_capacity(p_other.m_capacity) {}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_map	   = std::move(p_other.m_map);
				m_keys	   = std::move(p_other.m_keys);
				m_capacity = p_other.m_capacity;
			}
			return *this;
		}

		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			auto map_iter = m_map.find(p_key);
			if (map_iter != m_map.end())
			{
				map_iter->second.first = p_value;
				return;
			}

			if (m_map.size() >= m_capacity && !m_keys.empty())
			{
				// Optimized random eviction: swap-and-pop for O(1) removal
				const std::size_t random_index = static_cast<std::size_t>(std::rand()) % m_keys.size();
				const key_t victim_key		   = m_keys[random_index];

				// Update the index mapping for the swapped element
				if (random_index < m_keys.size() - 1)
				{
					const key_t last_key   = m_keys.back();
					m_keys[random_index]   = last_key;
					m_map[last_key].second = random_index;
				}

				m_keys.pop_back();
				m_map.erase(victim_key);
			}

			const std::size_t new_index = m_keys.size();
			m_map[p_key]				= {p_value, new_index};
			m_keys.push_back(p_key);
		}

		auto get(const key_t& p_key) -> value_t { return m_map.at(p_key).first; }

		// Additional utility methods
		auto contains(const key_t& p_key) const -> bool { return m_map.find(p_key) != m_map.end(); }

		auto size() const -> std::size_t { return m_map.size(); }

		auto empty() const -> bool { return m_map.empty(); }

		auto capacity() const -> std::size_t { return m_capacity; }

		auto clear() -> void
		{
			m_map.clear();
			m_keys.clear();
		}
	};

	/**
	 * @brief Policy-based cache implementation
	 *
	 * This class provides a flexible, policy-based cache that allows
	 * composition of different eviction, storage, access, and capacity policies.
	 *
	 * @tparam key_t The key type for cache entries
	 * @tparam value_t The value type for cache entries
	 * @tparam eviction_policy_t The eviction policy implementation
	 * @tparam storage_policy_t The storage policy implementation
	 * @tparam access_policy_t The access policy implementation
	 * @tparam capacity_policy_t The capacity policy implementation
	 */
	template <typename key_t, typename value_t, template <typename, typename> class eviction_policy_t, template <typename, typename> class storage_policy_t,
			  template <typename, typename> class access_policy_t, template <typename, typename> class capacity_policy_t>
	class policy_based_cache
	{
	  private:
		// Policy type definitions
		using eviction_policy_type = eviction_policy_t<key_t, value_t>;
		using storage_policy_type  = storage_policy_t<key_t, value_t>;
		using access_policy_type   = access_policy_t<key_t, value_t>;
		using capacity_policy_type = capacity_policy_t<key_t, value_t>;

		// Compile-time policy validation
		using policy_validator = policies::traits::policy_validator<key_t, value_t, eviction_policy_type, storage_policy_type, access_policy_type, capacity_policy_type>;

	  public:
		using self_t	 = policy_based_cache<key_t, value_t, eviction_policy_t, storage_policy_t, access_policy_t, capacity_policy_t>;
		using key_type	 = key_t;
		using value_type = value_t;

	  private:
		std::unique_ptr<eviction_policy_type> m_eviction_policy;
		std::unique_ptr<storage_policy_type> m_storage_policy;
		std::unique_ptr<access_policy_type> m_access_policy;
		std::unique_ptr<capacity_policy_type> m_capacity_policy;

	  public:
		// Destructor
		~policy_based_cache() {}

		// Constructor
		explicit policy_based_cache(std::size_t p_capacity)
			: m_eviction_policy(std::unique_ptr<eviction_policy_type>(new eviction_policy_type())), m_storage_policy(std::unique_ptr<storage_policy_type>(new storage_policy_type())),
			  m_access_policy(std::unique_ptr<access_policy_type>(new access_policy_type())), m_capacity_policy(std::unique_ptr<capacity_policy_type>(new capacity_policy_type(p_capacity)))
		{
		}

		// Deleted copy constructor and assignment operator
		policy_based_cache(const self_t&)		 = delete;
		auto operator=(const self_t&) -> self_t& = delete;

		// Move constructor and assignment operator
		policy_based_cache(self_t&& p_other) noexcept
			: m_eviction_policy(std::move(p_other.m_eviction_policy)), m_storage_policy(std::move(p_other.m_storage_policy)), m_access_policy(std::move(p_other.m_access_policy)),
			  m_capacity_policy(std::move(p_other.m_capacity_policy))
		{
		}

		auto operator=(self_t&& p_other) noexcept -> self_t&
		{
			if (this != &p_other)
			{
				m_eviction_policy = std::move(p_other.m_eviction_policy);
				m_storage_policy  = std::move(p_other.m_storage_policy);
				m_access_policy	  = std::move(p_other.m_access_policy);
				m_capacity_policy = std::move(p_other.m_capacity_policy);
			}
			return *this;
		}

		/**
		 * @brief Insert or update a key-value pair
		 *
		 * If the key already exists, updates the value and notifies
		 * the eviction policy. If the key is new and the cache is full,
		 * evicts entries according to the eviction policy.
		 *
		 * @param p_key The key to insert/update
		 * @param p_value The value to store
		 */
		auto put(const key_t& p_key, const value_t& p_value) -> void
		{
			// Check if key already exists
			const bool is_new_key = !m_storage_policy->contains(p_key);

			if (is_new_key)
			{
				// Handle capacity constraints for new key
				this->evict_if_necessary_for_insertion();

				// Insert new key-value pair
				m_storage_policy->insert(p_key, p_value);
				m_eviction_policy->on_insert(p_key);
			}
			else
			{
				// Update existing key-value pair
				m_storage_policy->insert(p_key, p_value);
				m_eviction_policy->on_update(p_key);
			}
		}

		/**
		 * @brief Retrieve a value by key
		 *
		 * If the key exists, returns the associated value and notifies
		 * the access policy. If the key doesn't exist, throws an exception.
		 *
		 * @param p_key The key to search for
		 * @return The associated value
		 * @throws std::out_of_range if the key is not found
		 */
		auto get(const key_t& p_key) -> value_t
		{
			value_t* p_value = m_storage_policy->find(p_key);

			if (p_value != nullptr)
			{
				// Handle access policy
				const bool should_update_eviction = m_access_policy->on_access(p_key, *m_eviction_policy);

				if (should_update_eviction)
				{
					m_eviction_policy->on_access(p_key);
				}

				return *p_value;
			}

			// Handle cache miss
			m_access_policy->on_miss(p_key);
			throw std::out_of_range("Key not found in cache");
		}

		/**
		 * @brief Check if a key exists in the cache
		 *
		 * @param p_key The key to check
		 * @return true if the key exists, false otherwise
		 */
		auto contains(const key_t& p_key) const -> bool { return m_storage_policy->contains(p_key); }

		/**
		 * @brief Get the number of entries in the cache
		 *
		 * @return The number of cached entries
		 */
		auto size() const -> std::size_t { return m_storage_policy->size(); }

		/**
		 * @brief Check if the cache is empty
		 *
		 * @return true if the cache contains no entries
		 */
		auto empty() const -> bool { return m_storage_policy->empty(); }

		/**
		 * @brief Get the current capacity limit
		 *
		 * @return The maximum number of entries allowed
		 */
		auto capacity() const -> std::size_t { return m_capacity_policy->capacity(); }

		/**
		 * @brief Set a new capacity limit
		 *
		 * If the new capacity is smaller than the current size,
		 * entries will be evicted according to the eviction policy.
		 *
		 * @param p_new_capacity The new capacity limit
		 */
		auto set_capacity(std::size_t p_new_capacity) -> void
		{
			m_capacity_policy->set_capacity(p_new_capacity);
			this->evict_if_necessary_for_capacity_change();
		}

		/**
		 * @brief Clear all entries from the cache
		 */
		auto clear() -> void
		{
			m_storage_policy->clear();
			m_eviction_policy->clear();
		}

		/**
		 * @brief Remove a specific key from the cache
		 *
		 * @param p_key The key to remove
		 * @return true if the key was found and removed, false otherwise
		 */
		auto erase(const key_t& p_key) -> bool
		{
			const bool was_erased = m_storage_policy->erase(p_key);

			if (was_erased)
			{
				m_eviction_policy->remove_key(p_key);
			}

			return was_erased;
		}

	  private:
		/**
		 * @brief Evict entries if necessary before inserting a new key
		 */
		auto evict_if_necessary_for_insertion() -> void
		{
			const std::size_t current_size = m_storage_policy->size();

			if (m_capacity_policy->needs_eviction(current_size))
			{
				const std::size_t eviction_count = m_capacity_policy->eviction_count(current_size);
				this->evict_entries(eviction_count);
			}
		}

		/**
		 * @brief Evict entries if necessary after capacity change
		 */
		auto evict_if_necessary_for_capacity_change() -> void
		{
			const std::size_t current_size = m_storage_policy->size();

			if (m_capacity_policy->needs_eviction(current_size))
			{
				const std::size_t eviction_count = m_capacity_policy->eviction_count(current_size);
				this->evict_entries(eviction_count);
			}
		}

		/**
		 * @brief Evict a specified number of entries
		 *
		 * @param p_count The number of entries to evict
		 */
		auto evict_entries(std::size_t p_count) -> void
		{
			for (std::size_t idx_for = 0; idx_for < p_count && !m_storage_policy->empty(); ++idx_for)
			{
				try
				{
					const key_t victim_key = m_eviction_policy->select_victim();
					const bool was_erased  = m_storage_policy->erase(victim_key);

					if (was_erased)
					{
						m_eviction_policy->remove_key(victim_key);
					}
					else
					{
						// This shouldn't happen in a correct implementation
						throw policies::policy_error("Eviction policy selected non-existent key");
					}
				}
				catch (const std::runtime_error& p_error)
				{
					// If we can't evict more entries, break the loop
					break;
				}
			}
		}

	  public:
		/**
		 * @brief Get access to the eviction policy (for advanced use cases)
		 *
		 * @return Reference to the eviction policy
		 */
		auto eviction_policy() -> eviction_policy_type& { return *m_eviction_policy; }

		/**
		 * @brief Get access to the eviction policy (const version)
		 *
		 * @return Const reference to the eviction policy
		 */
		auto eviction_policy() const -> const eviction_policy_type& { return *m_eviction_policy; }

		/**
		 * @brief Get access to the storage policy (for advanced use cases)
		 *
		 * @return Reference to the storage policy
		 */
		auto storage_policy() -> storage_policy_type& { return *m_storage_policy; }

		/**
		 * @brief Get access to the storage policy (const version)
		 *
		 * @return Const reference to the storage policy
		 */
		auto storage_policy() const -> const storage_policy_type& { return *m_storage_policy; }

		/**
		 * @brief Get access to the access policy (for advanced use cases)
		 *
		 * @return Reference to the access policy
		 */
		auto access_policy() -> access_policy_type& { return *m_access_policy; }

		/**
		 * @brief Get access to the access policy (const version)
		 *
		 * @return Const reference to the access policy
		 */
		auto access_policy() const -> const access_policy_type& { return *m_access_policy; }

		/**
		 * @brief Get access to the capacity policy (for advanced use cases)
		 *
		 * @return Reference to the capacity policy
		 */
		auto capacity_policy() -> capacity_policy_type& { return *m_capacity_policy; }

		/**
		 * @brief Get access to the capacity policy (const version)
		 *
		 * @return Const reference to the capacity policy
		 */
		auto capacity_policy() const -> const capacity_policy_type& { return *m_capacity_policy; }
	};

	/**
	 * @brief Factory function to create caches from policy sets
	 *
	 * @tparam policy_set_t A tuple containing policy types
	 * @tparam key_t The key type
	 * @tparam value_t The value type
	 * @param p_capacity The initial cache capacity
	 * @return A policy-based cache with the specified policy set
	 */
	template <template <typename, typename> class policy_set_t, typename key_t, typename value_t>
	auto make_cache(std::size_t p_capacity) -> policy_based_cache<key_t, value_t,
																  policy_templates::lru_eviction, // Default to LRU for now
																  policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>(
			p_capacity);
	}

	/**
	 * @brief Convenience factory for LRU cache
	 */
	template <typename key_t, typename value_t>
	auto make_lru_cache(std::size_t p_capacity)
		-> policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>(
			p_capacity);
	}

	/**
	 * @brief Convenience factory for FIFO cache
	 */
	template <typename key_t, typename value_t>
	auto make_fifo_cache(std::size_t p_capacity)
		-> policy_based_cache<key_t, value_t, policy_templates::fifo_eviction, policy_templates::hash_storage, policy_templates::no_update_on_access, policy_templates::fixed_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::fifo_eviction, policy_templates::hash_storage, policy_templates::no_update_on_access, policy_templates::fixed_capacity>(
			p_capacity);
	}

	/**
	 * @brief Convenience factory for LFU cache
	 */
	template <typename key_t, typename value_t>
	auto make_lfu_cache(std::size_t p_capacity)
		-> policy_based_cache<key_t, value_t, policy_templates::lfu_eviction, policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::lfu_eviction, policy_templates::hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>(
			p_capacity);
	}

	/**
	 * @brief Convenience factory for high-performance cache
	 */
	template <typename key_t, typename value_t>
	auto make_high_performance_cache(std::size_t p_capacity)
		-> policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::reserved_hash_storage, policy_templates::update_on_access, policy_templates::fixed_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::reserved_hash_storage, policy_templates::update_on_access,
								  policy_templates::fixed_capacity>(p_capacity);
	}

	/**
	 * @brief Convenience factory for memory-efficient cache
	 */
	template <typename key_t, typename value_t>
	auto make_memory_efficient_cache(std::size_t p_memory_limit)
		-> policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::compact_storage, policy_templates::update_on_access, policy_templates::memory_capacity>
	{
		return policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::compact_storage, policy_templates::update_on_access, policy_templates::memory_capacity>(
			p_memory_limit);
	}

	/**
	 * @brief Convenience factory for adaptive cache
	 */
	template <typename key_t, typename value_t>
	auto make_adaptive_cache(std::size_t p_base_capacity, std::size_t p_min_capacity = default_min_capacity, std::size_t p_max_capacity = default_max_capacity)
		-> policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::hash_storage, policy_templates::threshold_access, policy_templates::dynamic_capacity>
	{
		auto cache_instance =
			policy_based_cache<key_t, value_t, policy_templates::lru_eviction, policy_templates::hash_storage, policy_templates::threshold_access, policy_templates::dynamic_capacity>(
				p_base_capacity);

		cache_instance.capacity_policy().set_capacity_bounds(p_min_capacity, p_max_capacity);
		return cache_instance;
	}

	// Convenience factory functions for template specialization caches
	template <typename key_t, typename value_t> auto make_lru_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::lru>
	{
		return cache<key_t, value_t, algorithm::lru>(p_capacity);
	}

	template <typename key_t, typename value_t> auto make_fifo_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::fifo>
	{
		return cache<key_t, value_t, algorithm::fifo>(p_capacity);
	}

	template <typename key_t, typename value_t> auto make_lfu_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::lfu>
	{
		return cache<key_t, value_t, algorithm::lfu>(p_capacity);
	}

	template <typename key_t, typename value_t> auto make_mfu_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::mfu>
	{
		return cache<key_t, value_t, algorithm::mfu>(p_capacity);
	}

	template <typename key_t, typename value_t> auto make_mru_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::mru>
	{
		return cache<key_t, value_t, algorithm::mru>(p_capacity);
	}

	template <typename key_t, typename value_t> auto make_random_cache_spec(std::size_t p_capacity) -> cache<key_t, value_t, algorithm::random_cache>
	{
		return cache<key_t, value_t, algorithm::random_cache>(p_capacity);
	}

} // namespace cache_engine

/**
 * @brief Documentation examples and usage patterns
 *
 * Example 1: Basic usage (backward compatible)
 * ```cpp
 * #include "cache_engine/cache_new.hpp"
 *
 * // Old way (still works)
 * cache_engine::cache<int, std::string, cache_engine::algorithm::lru> old_cache(100);
 * old_cache.put(1, "hello");
 * std::string value = old_cache.get(1);
 * ```
 *
 * Example 2: New policy-based approach
 * ```cpp
 * #include "cache_engine/cache_new.hpp"
 *
 * // New way with explicit policies
 * auto cache = cache_engine::policy_based_cache<
 *     int, std::string,
 *     cache_engine::policy_templates::lru_eviction,
 *     cache_engine::policy_templates::hash_storage,
 *     cache_engine::policy_templates::update_on_access,
 *     cache_engine::policy_templates::fixed_capacity
 * >(100);
 *
 * cache.put(1, "hello");
 * std::string value = cache.get(1);
 * bool exists = cache.contains(1);
 * std::size_t size = cache.size();
 * ```
 *
 * Example 3: Using convenience factories
 * ```cpp
 * #include "cache_engine/cache_new.hpp"
 *
 * // Easy factory functions
 * auto lru_cache = cache_engine::make_lru_cache<int, std::string>(100);
 * auto adaptive_cache = cache_engine::make_adaptive_cache<int, std::string>(100, 10, 1000);
 * auto memory_cache = cache_engine::make_memory_efficient_cache<int, std::string>(1024 * 1024); // 1MB
 * ```
 *
 * Example 4: Custom policy composition
 * ```cpp
 * #include "cache_engine/cache_new.hpp"
 *
 * // Custom combination: LFU eviction with soft capacity and time decay
 * auto custom_cache = cache_engine::policy_based_cache<
 *     int, std::string,
 *     cache_engine::policy_templates::lfu_eviction,
 *     cache_engine::policy_templates::hash_storage,
 *     cache_engine::policy_templates::time_decay_access,
 *     cache_engine::policy_templates::soft_capacity
 * >(100);
 *
 * // Configure policies
 * custom_cache.access_policy().set_decay_interval(50);
 * custom_cache.capacity_policy().set_overage_tolerance(0.3); // 30% overage allowed
 * ```
 *
 * Example 5: Debug and monitoring
 * ```cpp
 * #include "cache_engine/cache_new.hpp"
 *
 * // Debug cache with statistics
 * auto debug_cache = cache_engine::policy_based_cache<
 *     int, std::string,
 *     cache_engine::policy_templates::lru_eviction,
 *     cache_engine::policy_templates::debug_storage,
 *     cache_engine::policy_templates::update_on_access,
 *     cache_engine::policy_templates::fixed_capacity
 * >(100);
 *
 * // Use cache...
 * debug_cache.put(1, "test");
 * debug_cache.get(1);
 *
 * // Check statistics
 * auto& storage = debug_cache.storage_policy();
 * std::size_t ops = storage.operation_count();
 * std::size_t hits = storage.hit_count();
 * double hit_ratio = storage.hit_ratio();
 * ```
 */