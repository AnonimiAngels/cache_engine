// File: inc/cache_engine/policies/policy_traits.hpp

#pragma once

#include <cstddef>
#include <type_traits>

namespace cache_engine
{
	namespace policies
	{
		// Forward declarations
		template <typename key_t, typename value_t> class eviction_policy_base;
		template <typename key_t, typename value_t> class storage_policy_base;
		template <typename key_t, typename value_t> class access_policy_base;
		template <typename key_t, typename value_t> class capacity_policy_base;

		namespace traits
		{
			/**
			 * @brief SFINAE helper for detecting valid key types
			 *
			 * A valid key type must be:
			 * - Copy constructible
			 * - Copy assignable
			 * - Equality comparable
			 * - Hashable (for unordered containers)
			 */
			template <typename key_t> struct is_valid_key_type
			{
			  private:
				template <typename t> static auto test_copy_constructible(int) -> decltype(t(std::declval<const t&>()), std::true_type{});
				template <typename> static auto test_copy_constructible(...) -> std::false_type;

				template <typename t> static auto test_copy_assignable(int) -> decltype(std::declval<t&>() = std::declval<const t&>(), std::true_type{});
				template <typename> static auto test_copy_assignable(...) -> std::false_type;

				template <typename t> static auto test_equality_comparable(int) -> decltype(std::declval<const t&>() == std::declval<const t&>(), std::true_type{});
				template <typename> static auto test_equality_comparable(...) -> std::false_type;

			  public:
				static constexpr bool value =
					decltype(test_copy_constructible<key_t>(0))::value && decltype(test_copy_assignable<key_t>(0))::value && decltype(test_equality_comparable<key_t>(0))::value;
			};

			/**
			 * @brief SFINAE helper for detecting valid value types
			 *
			 * A valid value type must be:
			 * - Copy constructible
			 * - Copy assignable
			 */
			template <typename value_t> struct is_valid_value_type
			{
			  private:
				template <typename t> static auto test_copy_constructible(int) -> decltype(t(std::declval<const t&>()), std::true_type{});
				template <typename> static auto test_copy_constructible(...) -> std::false_type;

				template <typename t> static auto test_copy_assignable(int) -> decltype(std::declval<t&>() = std::declval<const t&>(), std::true_type{});
				template <typename> static auto test_copy_assignable(...) -> std::false_type;

			  public:
				static constexpr bool value = decltype(test_copy_constructible<value_t>(0))::value && decltype(test_copy_assignable<value_t>(0))::value;
			};

			/**
			 * @brief SFINAE helper for detecting eviction policy interface compliance
			 */
			template <typename policy_t, typename key_t, typename value_t> struct is_eviction_policy
			{
			  private:
				template <typename p> static auto test_base_class(int) -> decltype(static_cast<eviction_policy_base<key_t, value_t>*>(static_cast<p*>(nullptr)), std::true_type{});
				template <typename> static auto test_base_class(...) -> std::false_type;

				template <typename p> static auto test_on_access(int) -> decltype(std::declval<p>().on_access(std::declval<const key_t&>()), std::true_type{});
				template <typename> static auto test_on_access(...) -> std::false_type;

				template <typename p> static auto test_on_insert(int) -> decltype(std::declval<p>().on_insert(std::declval<const key_t&>()), std::true_type{});
				template <typename> static auto test_on_insert(...) -> std::false_type;

				template <typename p> static auto test_select_victim(int) -> decltype(std::declval<p>().select_victim(), std::true_type{});
				template <typename> static auto test_select_victim(...) -> std::false_type;

			  public:
				static constexpr bool value = decltype(test_base_class<policy_t>(0))::value && decltype(test_on_access<policy_t>(0))::value && decltype(test_on_insert<policy_t>(0))::value
											  && decltype(test_select_victim<policy_t>(0))::value;
			};

			/**
			 * @brief SFINAE helper for detecting storage policy interface compliance
			 */
			template <typename policy_t, typename key_t, typename value_t> struct is_storage_policy
			{
			  private:
				template <typename p> static auto test_base_class(int) -> decltype(static_cast<storage_policy_base<key_t, value_t>*>(static_cast<p*>(nullptr)), std::true_type{});
				template <typename> static auto test_base_class(...) -> std::false_type;

				template <typename p> static auto test_insert(int) -> decltype(std::declval<p>().insert(std::declval<const key_t&>(), std::declval<const value_t&>()), std::true_type{});
				template <typename> static auto test_insert(...) -> std::false_type;

				template <typename p> static auto test_find(int) -> decltype(std::declval<p>().find(std::declval<const key_t&>()), std::true_type{});
				template <typename> static auto test_find(...) -> std::false_type;

				template <typename p> static auto test_erase(int) -> decltype(std::declval<p>().erase(std::declval<const key_t&>()), std::true_type{});
				template <typename> static auto test_erase(...) -> std::false_type;

			  public:
				static constexpr bool value = decltype(test_base_class<policy_t>(0))::value && decltype(test_insert<policy_t>(0))::value && decltype(test_find<policy_t>(0))::value
											  && decltype(test_erase<policy_t>(0))::value;
			};

			/**
			 * @brief SFINAE helper for detecting access policy interface compliance
			 */
			template <typename policy_t, typename key_t, typename value_t> struct is_access_policy
			{
			  private:
				template <typename p> static auto test_base_class(int) -> decltype(static_cast<access_policy_base<key_t, value_t>*>(static_cast<p*>(nullptr)), std::true_type{});
				template <typename> static auto test_base_class(...) -> std::false_type;

				template <typename p>
				static auto test_on_access(int)
					-> decltype(std::declval<p>().on_access(std::declval<const key_t&>(), std::declval<eviction_policy_base<key_t, value_t>&>()), std::true_type{});
				template <typename> static auto test_on_access(...) -> std::false_type;

			  public:
				static constexpr bool value = decltype(test_base_class<policy_t>(0))::value && decltype(test_on_access<policy_t>(0))::value;
			};

			/**
			 * @brief SFINAE helper for detecting capacity policy interface compliance
			 */
			template <typename policy_t, typename key_t, typename value_t> struct is_capacity_policy
			{
			  private:
				template <typename p> static auto test_base_class(int) -> decltype(static_cast<capacity_policy_base<key_t, value_t>*>(static_cast<p*>(nullptr)), std::true_type{});
				template <typename> static auto test_base_class(...) -> std::false_type;

				template <typename p> static auto test_capacity(int) -> decltype(std::declval<const p>().capacity(), std::true_type{});
				template <typename> static auto test_capacity(...) -> std::false_type;

				template <typename p> static auto test_needs_eviction(int) -> decltype(std::declval<const p>().needs_eviction(std::declval<std::size_t>()), std::true_type{});
				template <typename> static auto test_needs_eviction(...) -> std::false_type;

			  public:
				static constexpr bool value =
					decltype(test_base_class<policy_t>(0))::value && decltype(test_capacity<policy_t>(0))::value && decltype(test_needs_eviction<policy_t>(0))::value;
			};

			/**
			 * @brief Helper to check if all policies are compatible
			 */
			template <typename key_t, typename value_t, typename eviction_policy_t, typename storage_policy_t, typename access_policy_t, typename capacity_policy_t>
			struct are_policies_compatible
			{
				static constexpr bool value = is_valid_key_type<key_t>::value && is_valid_value_type<value_t>::value && is_eviction_policy<eviction_policy_t, key_t, value_t>::value
											  && is_storage_policy<storage_policy_t, key_t, value_t>::value && is_access_policy<access_policy_t, key_t, value_t>::value
											  && is_capacity_policy<capacity_policy_t, key_t, value_t>::value;
			};

			/**
			 * @brief Enable if template for policy validation
			 *
			 * Used to conditionally enable template instantiations
			 * based on policy compatibility.
			 */
			template <typename key_t, typename value_t, typename eviction_policy_t, typename storage_policy_t, typename access_policy_t, typename capacity_policy_t>
			using enable_if_policies_compatible =
				typename std::enable_if<are_policies_compatible<key_t, value_t, eviction_policy_t, storage_policy_t, access_policy_t, capacity_policy_t>::value>::type;

			/**
			 * @brief Compile-time policy validation
			 *
			 * Provides readable error messages when policies are incompatible.
			 */
			template <typename key_t, typename value_t, typename eviction_policy_t, typename storage_policy_t, typename access_policy_t, typename capacity_policy_t> struct policy_validator
			{
				static_assert(is_valid_key_type<key_t>::value, "Key type must be copy constructible, copy assignable, and equality comparable");

				static_assert(is_valid_value_type<value_t>::value, "Value type must be copy constructible and copy assignable");

				static_assert(is_eviction_policy<eviction_policy_t, key_t, value_t>::value, "Eviction policy must inherit from eviction_policy_base and implement required interface");

				static_assert(is_storage_policy<storage_policy_t, key_t, value_t>::value, "Storage policy must inherit from storage_policy_base and implement required interface");

				static_assert(is_access_policy<access_policy_t, key_t, value_t>::value, "Access policy must inherit from access_policy_base and implement required interface");

				static_assert(is_capacity_policy<capacity_policy_t, key_t, value_t>::value, "Capacity policy must inherit from capacity_policy_base and implement required interface");
			};

			/**
			 * @brief Policy performance characteristics
			 *
			 * Compile-time information about policy performance.
			 * Used for optimization and documentation.
			 */
			template <typename policy_t> struct policy_characteristics
			{
				// Default characteristics - should be specialized for each policy
				static constexpr std::size_t access_complexity	 = 1; // O(1)
				static constexpr std::size_t eviction_complexity = 1; // O(1)
				static constexpr std::size_t memory_overhead	 = sizeof(policy_t);
				static constexpr bool is_order_dependent		 = false;
				static constexpr bool is_frequency_dependent	 = false;
			};

		} // namespace traits
	} // namespace policies
} // namespace cache_engine