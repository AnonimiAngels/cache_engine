// File: inc/cache_engine/policies/policy_interfaces.hpp

#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

namespace cache_engine
{
	namespace policies
	{
		// Forward declarations for policy concepts
		template <typename key_t, typename value_t> class eviction_policy_base;
		template <typename key_t, typename value_t> class storage_policy_base;
		template <typename key_t, typename value_t> class access_policy_base;
		template <typename key_t, typename value_t> class capacity_policy_base;

		/**
		 * @brief Base interface for eviction policies
		 *
		 * Defines the contract for cache eviction algorithms.
		 * All eviction policies must inherit from this interface.
		 *
		 * @tparam key_t The key type for cache entries
		 * @tparam value_t The value type for cache entries
		 */
		template <typename key_t, typename value_t> class eviction_policy_base
		{
		  public:
			using self_t	 = eviction_policy_base<key_t, value_t>;
			using key_type	 = key_t;
			using value_type = value_t;

		  public:
			// Virtual destructor for proper cleanup
			virtual ~eviction_policy_base() = default;

			// Deleted copy constructor and assignment operator
			eviction_policy_base(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			eviction_policy_base(self_t&& p_other) noexcept		 = default;
			auto operator=(self_t&& p_other) noexcept -> self_t& = default;

		  protected:
			// Default constructor for derived classes
			eviction_policy_base() = default;

		  public:
			/**
			 * @brief Called when a key is accessed (get operation)
			 * @param p_key The key being accessed
			 */
			virtual auto on_access(const key_t& p_key) -> void = 0;

			/**
			 * @brief Called when a new key is inserted
			 * @param p_key The key being inserted
			 */
			virtual auto on_insert(const key_t& p_key) -> void = 0;

			/**
			 * @brief Called when a key is updated (put operation on existing key)
			 * @param p_key The key being updated
			 */
			virtual auto on_update(const key_t& p_key) -> void = 0;

			/**
			 * @brief Select a victim key for eviction when cache is full
			 * @return The key to be evicted
			 * @throws std::runtime_error if no victim can be selected
			 */
			virtual auto select_victim() -> key_t = 0;

			/**
			 * @brief Remove a key from eviction tracking
			 * @param p_key The key to remove
			 */
			virtual auto remove_key(const key_t& p_key) -> void = 0;

			/**
			 * @brief Check if the eviction policy is empty
			 * @return true if no keys are being tracked
			 */
			virtual auto empty() const -> bool = 0;

			/**
			 * @brief Get the number of keys being tracked
			 * @return The number of tracked keys
			 */
			virtual auto size() const -> std::size_t = 0;

			/**
			 * @brief Clear all tracked keys
			 */
			virtual auto clear() -> void = 0;
		};

		/**
		 * @brief Base interface for storage policies
		 *
		 * Defines the contract for cache storage mechanisms.
		 * All storage policies must inherit from this interface.
		 *
		 * @tparam key_t The key type for cache entries
		 * @tparam value_t The value type for cache entries
		 */
		template <typename key_t, typename value_t> class storage_policy_base
		{
		  public:
			using self_t	 = storage_policy_base<key_t, value_t>;
			using key_type	 = key_t;
			using value_type = value_t;

		  public:
			// Virtual destructor for proper cleanup
			virtual ~storage_policy_base() = default;

			// Deleted copy constructor and assignment operator
			storage_policy_base(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			storage_policy_base(self_t&& p_other) noexcept		 = default;
			auto operator=(self_t&& p_other) noexcept -> self_t& = default;

		  protected:
			// Default constructor for derived classes
			storage_policy_base() = default;

		  public:
			/**
			 * @brief Insert or update a key-value pair
			 * @param p_key The key to insert/update
			 * @param p_value The value to store
			 * @return true if this was a new insertion, false if it was an update
			 */
			virtual auto insert(const key_t& p_key, const value_t& p_value) -> bool = 0;

			/**
			 * @brief Find a value by key
			 * @param p_key The key to search for
			 * @return Pointer to the value if found, nullptr otherwise
			 */
			virtual auto find(const key_t& p_key) -> value_t* = 0;

			/**
			 * @brief Find a value by key (const version)
			 * @param p_key The key to search for
			 * @return Pointer to the value if found, nullptr otherwise
			 */
			virtual auto find(const key_t& p_key) const -> const value_t* = 0;

			/**
			 * @brief Remove a key-value pair
			 * @param p_key The key to remove
			 * @return true if the key was found and removed, false otherwise
			 */
			virtual auto erase(const key_t& p_key) -> bool = 0;

			/**
			 * @brief Check if a key exists
			 * @param p_key The key to check
			 * @return true if the key exists, false otherwise
			 */
			virtual auto contains(const key_t& p_key) const -> bool = 0;

			/**
			 * @brief Get the number of stored key-value pairs
			 * @return The number of entries
			 */
			virtual auto size() const -> std::size_t = 0;

			/**
			 * @brief Check if the storage is empty
			 * @return true if no entries are stored
			 */
			virtual auto empty() const -> bool = 0;

			/**
			 * @brief Clear all stored entries
			 */
			virtual auto clear() -> void = 0;
		};

		/**
		 * @brief Base interface for access policies
		 *
		 * Defines the contract for handling cache access patterns.
		 * Controls whether and how access operations affect eviction order.
		 *
		 * @tparam key_t The key type for cache entries
		 * @tparam value_t The value type for cache entries
		 */
		template <typename key_t, typename value_t> class access_policy_base
		{
		  public:
			using self_t	 = access_policy_base<key_t, value_t>;
			using key_type	 = key_t;
			using value_type = value_t;

		  public:
			// Virtual destructor for proper cleanup
			virtual ~access_policy_base() = default;

			// Deleted copy constructor and assignment operator
			access_policy_base(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			access_policy_base(self_t&& p_other) noexcept		 = default;
			auto operator=(self_t&& p_other) noexcept -> self_t& = default;

		  protected:
			// Default constructor for derived classes
			access_policy_base() = default;

		  public:
			/**
			 * @brief Handle a cache access operation
			 * @param p_key The key being accessed
			 * @param p_eviction_policy Reference to the eviction policy
			 * @return true if the access should update eviction order
			 */
			virtual auto on_access(const key_t& p_key, eviction_policy_base<key_t, value_t>& p_eviction_policy) -> bool = 0;

			/**
			 * @brief Handle a cache miss operation
			 * @param p_key The key that was not found
			 * @return true if the miss should be recorded
			 */
			virtual auto on_miss(const key_t& p_key) -> bool = 0;
		};

		/**
		 * @brief Base interface for capacity policies
		 *
		 * Defines the contract for managing cache capacity.
		 * Controls capacity limits and growth strategies.
		 *
		 * @tparam key_t The key type for cache entries
		 * @tparam value_t The value type for cache entries
		 */
		template <typename key_t, typename value_t> class capacity_policy_base
		{
		  public:
			using self_t	 = capacity_policy_base<key_t, value_t>;
			using key_type	 = key_t;
			using value_type = value_t;

		  public:
			// Virtual destructor for proper cleanup
			virtual ~capacity_policy_base() = default;

			// Deleted copy constructor and assignment operator
			capacity_policy_base(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			capacity_policy_base(self_t&& p_other) noexcept		 = default;
			auto operator=(self_t&& p_other) noexcept -> self_t& = default;

		  protected:
			// Default constructor for derived classes
			capacity_policy_base() = default;

		  public:
			/**
			 * @brief Get the current capacity limit
			 * @return The maximum number of entries allowed
			 */
			virtual auto capacity() const -> std::size_t = 0;

			/**
			 * @brief Set a new capacity limit
			 * @param p_new_capacity The new capacity limit
			 */
			virtual auto set_capacity(std::size_t p_new_capacity) -> void = 0;

			/**
			 * @brief Check if eviction is needed
			 * @param p_current_size The current number of entries
			 * @return true if eviction is required
			 */
			virtual auto needs_eviction(std::size_t p_current_size) const -> bool = 0;

			/**
			 * @brief Determine how many entries need to be evicted
			 * @param p_current_size The current number of entries
			 * @return The number of entries to evict
			 */
			virtual auto eviction_count(std::size_t p_current_size) const -> std::size_t = 0;
		};

		/**
		 * @brief Exception thrown when policy operations fail
		 */
		class policy_error : public std::runtime_error
		{
		  public:
			explicit policy_error(const std::string& p_message) : std::runtime_error("Policy Error: " + p_message) {}
		};

		/**
		 * @brief Exception thrown when cache operations fail
		 */
		class cache_error : public std::runtime_error
		{
		  public:
			explicit cache_error(const std::string& p_message) : std::runtime_error("Cache Error: " + p_message) {}
		};

	} // namespace policies
} // namespace cache_engine