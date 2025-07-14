#pragma once

#include "policy_interfaces.hpp"
#include <unordered_map>
#include <stdexcept>

namespace cache_engine
{
	namespace policies
	{
		/**
		 * @brief Hash table based storage policy
		 *
		 * Provides fast O(1) average-case storage operations using
		 * std::unordered_map. Suitable for most cache algorithms.
		 *
		 * Time Complexity:
		 * - insert: O(1) average, O(n) worst case
		 * - find: O(1) average, O(n) worst case
		 * - erase: O(1) average, O(n) worst case
		 * - contains: O(1) average, O(n) worst case
		 *
		 * Space Complexity: O(n) where n is number of entries
		 */
		template <typename key_t, typename value_t> class hash_storage_policy : public storage_policy_base<key_t, value_t>
		{
		  public:
			using self_t = hash_storage_policy<key_t, value_t>;
			using base_t = storage_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, value_t> m_storage;

		  public:
			// Constructor
			hash_storage_policy() = default;

			// Destructor
			~hash_storage_policy() override = default;

			// Move constructor and assignment operator
			hash_storage_policy(self_t&& p_other) noexcept : m_storage(std::move(p_other.m_storage)) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_storage = std::move(p_other.m_storage);
				}
				return *this;
			}

		  public:
			auto insert(const key_t& p_key, const value_t& p_value) -> bool override
			{
				const bool is_new_key = (m_storage.find(p_key) == m_storage.end());
				m_storage[p_key]	  = p_value;
				return is_new_key;
			}

			auto find(const key_t& p_key) -> value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto find(const key_t& p_key) const -> const value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto erase(const key_t& p_key) -> bool override { return m_storage.erase(p_key) > 0; }

			auto contains(const key_t& p_key) const -> bool override { return m_storage.find(p_key) != m_storage.end(); }

			auto size() const -> std::size_t override { return m_storage.size(); }

			auto empty() const -> bool override { return m_storage.empty(); }

			auto clear() -> void override { m_storage.clear(); }
		};

		/**
		 * @brief Hash table storage policy with pre-allocated capacity
		 *
		 * Similar to hash_storage_policy but reserves capacity upfront
		 * to reduce rehashing overhead for known cache sizes.
		 *
		 * Time Complexity: Same as hash_storage_policy
		 * Space Complexity: O(capacity) where capacity >= n
		 */
		template <typename key_t, typename value_t> class reserved_hash_storage_policy : public storage_policy_base<key_t, value_t>
		{
		  public:
			using self_t = reserved_hash_storage_policy<key_t, value_t>;
			using base_t = storage_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, value_t> m_storage;
			std::size_t m_reserved_capacity;

		  public:
			// Constructor with capacity hint
			explicit reserved_hash_storage_policy(std::size_t p_capacity = 100) : m_reserved_capacity(p_capacity) { m_storage.reserve(m_reserved_capacity); }

			// Destructor
			~reserved_hash_storage_policy() override = default;

			// Move constructor and assignment operator
			reserved_hash_storage_policy(self_t&& p_other) noexcept : m_storage(std::move(p_other.m_storage)), m_reserved_capacity(p_other.m_reserved_capacity) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_storage			= std::move(p_other.m_storage);
					m_reserved_capacity = p_other.m_reserved_capacity;
				}
				return *this;
			}

		  public:
			auto insert(const key_t& p_key, const value_t& p_value) -> bool override
			{
				const bool is_new_key = (m_storage.find(p_key) == m_storage.end());
				m_storage[p_key]	  = p_value;
				return is_new_key;
			}

			auto find(const key_t& p_key) -> value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto find(const key_t& p_key) const -> const value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto erase(const key_t& p_key) -> bool override { return m_storage.erase(p_key) > 0; }

			auto contains(const key_t& p_key) const -> bool override { return m_storage.find(p_key) != m_storage.end(); }

			auto size() const -> std::size_t override { return m_storage.size(); }

			auto empty() const -> bool override { return m_storage.empty(); }

			auto clear() -> void override
			{
				m_storage.clear();
				m_storage.reserve(m_reserved_capacity);
			}

		  public:
			/**
			 * @brief Set the reserved capacity for the hash table
			 * @param p_capacity The new capacity to reserve
			 */
			auto set_reserved_capacity(std::size_t p_capacity) -> void
			{
				m_reserved_capacity = p_capacity;
				m_storage.reserve(m_reserved_capacity);
			}

			/**
			 * @brief Get the current reserved capacity
			 * @return The reserved capacity
			 */
			auto reserved_capacity() const -> std::size_t { return m_reserved_capacity; }
		};

		/**
		 * @brief Compact storage policy for memory-constrained environments
		 *
		 * Uses memory-efficient data structures at the cost of some
		 * performance. Suitable for embedded systems or large-scale deployments.
		 *
		 * Time Complexity: Same as hash_storage_policy
		 * Space Complexity: O(n) with lower memory overhead
		 */
		template <typename key_t, typename value_t> class compact_storage_policy : public storage_policy_base<key_t, value_t>
		{
		  public:
			using self_t = compact_storage_policy<key_t, value_t>;
			using base_t = storage_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, value_t> m_storage;

		  public:
			// Constructor
			compact_storage_policy()
			{
				// Configure for memory efficiency
				m_storage.max_load_factor(0.75F); // Reduce memory usage
			}

			// Destructor
			~compact_storage_policy() override = default;

			// Move constructor and assignment operator
			compact_storage_policy(self_t&& p_other) noexcept : m_storage(std::move(p_other.m_storage)) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_storage = std::move(p_other.m_storage);
				}
				return *this;
			}

		  public:
			auto insert(const key_t& p_key, const value_t& p_value) -> bool override
			{
				const bool is_new_key = (m_storage.find(p_key) == m_storage.end());
				m_storage[p_key]	  = p_value;
				return is_new_key;
			}

			auto find(const key_t& p_key) -> value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto find(const key_t& p_key) const -> const value_t* override
			{
				auto iter = m_storage.find(p_key);
				return (iter != m_storage.end()) ? &iter->second : nullptr;
			}

			auto erase(const key_t& p_key) -> bool override
			{
				const bool was_erased = m_storage.erase(p_key) > 0;

				// Periodically shrink to fit for memory efficiency
				if (m_storage.size() > 0 && m_storage.bucket_count() > m_storage.size() * 4)
				{
					// Rehash to reduce bucket count when storage is sparse
					m_storage.rehash(m_storage.size());
				}

				return was_erased;
			}

			auto contains(const key_t& p_key) const -> bool override { return m_storage.find(p_key) != m_storage.end(); }

			auto size() const -> std::size_t override { return m_storage.size(); }

			auto empty() const -> bool override { return m_storage.empty(); }

			auto clear() -> void override
			{
				m_storage.clear();
				m_storage.rehash(0); // Minimize memory usage
			}
		};

		/**
		 * @brief Debug storage policy with operation logging
		 *
		 * Wraps another storage policy and logs all operations.
		 * Useful for debugging and performance analysis.
		 *
		 * Time Complexity: Same as wrapped policy + O(1) logging overhead
		 * Space Complexity: Same as wrapped policy + O(log entries)
		 */
		template <typename key_t, typename value_t, template <typename, typename> class wrapped_policy_t = hash_storage_policy>
		class debug_storage_policy : public storage_policy_base<key_t, value_t>
		{
		  public:
			using self_t	= debug_storage_policy<key_t, value_t, wrapped_policy_t>;
			using base_t	= storage_policy_base<key_t, value_t>;
			using wrapped_t = wrapped_policy_t<key_t, value_t>;

		  private:
			std::unique_ptr<wrapped_t> m_wrapped_policy;
			mutable std::size_t m_operation_count;
			mutable std::size_t m_hit_count;
			mutable std::size_t m_miss_count;

		  public:
			// Constructor
			debug_storage_policy() : m_wrapped_policy(std::unique_ptr<wrapped_t>(new wrapped_t())), m_operation_count(0), m_hit_count(0), m_miss_count(0) {}

			// Destructor
			~debug_storage_policy() override = default;

			// Move constructor and assignment operator
			debug_storage_policy(self_t&& p_other) noexcept
				: m_wrapped_policy(std::move(p_other.m_wrapped_policy)), m_operation_count(p_other.m_operation_count), m_hit_count(p_other.m_hit_count), m_miss_count(p_other.m_miss_count)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_wrapped_policy  = std::move(p_other.m_wrapped_policy);
					m_operation_count = p_other.m_operation_count;
					m_hit_count		  = p_other.m_hit_count;
					m_miss_count	  = p_other.m_miss_count;
				}
				return *this;
			}

		  public:
			auto insert(const key_t& p_key, const value_t& p_value) -> bool override
			{
				++m_operation_count;
				return m_wrapped_policy->insert(p_key, p_value);
			}

			auto find(const key_t& p_key) -> value_t* override
			{
				++m_operation_count;
				value_t* p_result = m_wrapped_policy->find(p_key);

				if (p_result != nullptr)
				{
					++m_hit_count;
				}
				else
				{
					++m_miss_count;
				}

				return p_result;
			}

			auto find(const key_t& p_key) const -> const value_t* override
			{
				++m_operation_count;
				const value_t* p_result = m_wrapped_policy->find(p_key);

				if (p_result != nullptr)
				{
					++m_hit_count;
				}
				else
				{
					++m_miss_count;
				}

				return p_result;
			}

			auto erase(const key_t& p_key) -> bool override
			{
				++m_operation_count;
				return m_wrapped_policy->erase(p_key);
			}

			auto contains(const key_t& p_key) const -> bool override
			{
				++m_operation_count;
				const bool result = m_wrapped_policy->contains(p_key);

				if (result)
				{
					++m_hit_count;
				}
				else
				{
					++m_miss_count;
				}

				return result;
			}

			auto size() const -> std::size_t override { return m_wrapped_policy->size(); }

			auto empty() const -> bool override { return m_wrapped_policy->empty(); }

			auto clear() -> void override
			{
				++m_operation_count;
				m_wrapped_policy->clear();
			}

		  public:
			/**
			 * @brief Get the total number of operations performed
			 * @return The operation count
			 */
			auto operation_count() const -> std::size_t { return m_operation_count; }

			/**
			 * @brief Get the number of cache hits
			 * @return The hit count
			 */
			auto hit_count() const -> std::size_t { return m_hit_count; }

			/**
			 * @brief Get the number of cache misses
			 * @return The miss count
			 */
			auto miss_count() const -> std::size_t { return m_miss_count; }

			/**
			 * @brief Calculate the cache hit ratio
			 * @return The hit ratio as a value between 0.0 and 1.0
			 */
			auto hit_ratio() const -> double
			{
				const std::size_t total_lookups = m_hit_count + m_miss_count;
				return (total_lookups > 0) ? static_cast<double>(m_hit_count) / static_cast<double>(total_lookups) : 0.0;
			}

			/**
			 * @brief Reset all statistics
			 */
			auto reset_statistics() -> void
			{
				m_operation_count = 0;
				m_hit_count		  = 0;
				m_miss_count	  = 0;
			}
		};

	} // namespace policies
} // namespace cache_engine