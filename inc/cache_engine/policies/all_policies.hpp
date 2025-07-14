#pragma once

/**
 * @file all_policies.hpp
 * @brief Comprehensive header that includes all policy implementations
 *
 * This header provides convenient access to all cache policy implementations
 * and their corresponding type aliases for common use cases.
 */

#include "policy_interfaces.hpp"
#include "policy_traits.hpp"
#include "eviction_policies.hpp"
#include "storage_policies.hpp"
#include "access_policies.hpp"
#include "capacity_policies.hpp"

namespace cache_engine
{
	namespace policies
	{
		// Convenient type aliases for common policy combinations

		/**
		 * @brief Standard LRU policy set
		 * Eviction: LRU, Storage: Hash, Access: Update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using lru_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Standard FIFO policy set
		 * Eviction: FIFO, Storage: Hash, Access: No update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using fifo_policy_set =
			std::tuple<fifo_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, no_update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Standard LFU policy set
		 * Eviction: LFU, Storage: Hash, Access: Update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using lfu_policy_set =
			std::tuple<lfu_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Standard MFU policy set
		 * Eviction: MFU, Storage: Hash, Access: Update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using mfu_policy_set =
			std::tuple<mfu_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Standard MRU policy set
		 * Eviction: MRU, Storage: Hash, Access: Update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using mru_policy_set =
			std::tuple<mru_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Standard Random policy set
		 * Eviction: Random, Storage: Hash, Access: No update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using random_policy_set =
			std::tuple<random_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, no_update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief High-performance policy set for speed-critical applications
		 * Eviction: LRU, Storage: Reserved Hash, Access: Update on access, Capacity: Fixed
		 */
		template <typename key_t, typename value_t>
		using high_performance_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, reserved_hash_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, fixed_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Memory-efficient policy set for resource-constrained environments
		 * Eviction: LRU, Storage: Compact, Access: Update on access, Capacity: Memory-based
		 */
		template <typename key_t, typename value_t>
		using memory_efficient_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, compact_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, memory_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Adaptive policy set for dynamic workloads
		 * Eviction: LRU, Storage: Hash, Access: Threshold-based, Capacity: Dynamic
		 */
		template <typename key_t, typename value_t>
		using adaptive_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, threshold_access_policy<key_t, value_t>, dynamic_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Debug policy set for development and testing
		 * Eviction: LRU, Storage: Debug wrapper, Access: Update on access, Capacity: Soft
		 */
		template <typename key_t, typename value_t>
		using debug_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, debug_storage_policy<key_t, value_t>, update_on_access_policy<key_t, value_t>, soft_capacity_policy<key_t, value_t>>;

		/**
		 * @brief Time-sensitive policy set for temporal data
		 * Eviction: LRU, Storage: Hash, Access: Time decay, Capacity: Soft
		 */
		template <typename key_t, typename value_t>
		using time_sensitive_policy_set =
			std::tuple<lru_eviction_policy<key_t, value_t>, hash_storage_policy<key_t, value_t>, time_decay_access_policy<key_t, value_t>, soft_capacity_policy<key_t, value_t>>;

	} // namespace policies

	// Policy template aliases for easier usage
	namespace policy_templates
	{
		// Eviction policy templates
		template <typename key_t, typename value_t> using lru_eviction	  = policies::lru_eviction_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using mru_eviction	  = policies::mru_eviction_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using fifo_eviction	  = policies::fifo_eviction_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using lfu_eviction	  = policies::lfu_eviction_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using mfu_eviction	  = policies::mfu_eviction_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using random_eviction = policies::random_eviction_policy<key_t, value_t>;

		// Storage policy templates
		template <typename key_t, typename value_t> using hash_storage			= policies::hash_storage_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using reserved_hash_storage = policies::reserved_hash_storage_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using compact_storage		= policies::compact_storage_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using debug_storage			= policies::debug_storage_policy<key_t, value_t>;

		// Access policy templates
		template <typename key_t, typename value_t> using update_on_access	  = policies::update_on_access_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using no_update_on_access = policies::no_update_on_access_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using threshold_access	  = policies::threshold_access_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using time_decay_access	  = policies::time_decay_access_policy<key_t, value_t>;

		// Capacity policy templates
		template <typename key_t, typename value_t> using fixed_capacity   = policies::fixed_capacity_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using dynamic_capacity = policies::dynamic_capacity_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using soft_capacity	   = policies::soft_capacity_policy<key_t, value_t>;
		template <typename key_t, typename value_t> using memory_capacity  = policies::memory_capacity_policy<key_t, value_t>;

	} // namespace policy_templates
} // namespace cache_engine