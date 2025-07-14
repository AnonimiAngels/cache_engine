#pragma once

#include "policy_interfaces.hpp"
#include <algorithm>

namespace cache_engine
{
	namespace policies
	{
		/**
		 * @brief Fixed capacity policy with hard limits
		 *
		 * Maintains a fixed capacity limit. When the limit is reached,
		 * exactly one item must be evicted before a new item can be inserted.
		 */
		template <typename key_t, typename value_t> class fixed_capacity_policy : public capacity_policy_base<key_t, value_t>
		{
		  public:
			using self_t = fixed_capacity_policy<key_t, value_t>;
			using base_t = capacity_policy_base<key_t, value_t>;

		  private:
			static constexpr std::size_t default_capacity = 100;
			std::size_t m_capacity;

		  public:
			// Constructor
			explicit fixed_capacity_policy(std::size_t p_capacity = default_capacity) : m_capacity(p_capacity) {}

			// Destructor
			~fixed_capacity_policy() override = default;

			// Deleted copy constructor and assignment operator
			fixed_capacity_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			fixed_capacity_policy(self_t&& p_other) noexcept : m_capacity(p_other.m_capacity) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_capacity = p_other.m_capacity;
				}
				return *this;
			}
			auto capacity() const -> std::size_t override { return m_capacity; }

			auto set_capacity(std::size_t p_new_capacity) -> void override { m_capacity = p_new_capacity; }

			auto needs_eviction(std::size_t p_current_size) const -> bool override { return p_current_size >= m_capacity; }

			auto eviction_count(std::size_t p_current_size) const -> std::size_t override
			{
				if (p_current_size >= m_capacity)
				{
					return (p_current_size - m_capacity) + 1; // +1 for the new item
				}
				return 0;
			}
		};

		/**
		 * @brief Dynamic capacity policy with growth and shrinkage
		 *
		 * Adjusts capacity based on usage patterns. Can grow when hit rates
		 * are high and shrink when memory pressure is detected.
		 */
		template <typename key_t, typename value_t> class dynamic_capacity_policy : public capacity_policy_base<key_t, value_t>
		{
		  public:
			using self_t = dynamic_capacity_policy<key_t, value_t>;
			using base_t = capacity_policy_base<key_t, value_t>;

		  private:
			static constexpr std::size_t default_base_capacity		 = 100;
			static constexpr std::size_t default_min_capacity		 = 10;
			static constexpr std::size_t default_max_capacity		 = 1000;
			static constexpr double default_growth_factor			 = 1.5;
			static constexpr double default_shrink_factor			 = 0.75;
			static constexpr std::size_t default_adjustment_interval = 100;
			static constexpr double min_shrink_factor				 = 0.1;
			static constexpr double high_utilization_threshold		 = 0.9;
			static constexpr double low_utilization_threshold		 = 0.5;

			std::size_t m_base_capacity;
			std::size_t m_current_capacity;
			std::size_t m_min_capacity;
			std::size_t m_max_capacity;
			double m_growth_factor;
			double m_shrink_factor;
			std::size_t m_adjustment_counter{0};
			std::size_t m_adjustment_interval;

		  public:
			// Constructor with configuration
			explicit dynamic_capacity_policy(std::size_t p_base_capacity = default_base_capacity, std::size_t p_min_capacity = default_min_capacity,
											 std::size_t p_max_capacity = default_max_capacity, double p_growth_factor = default_growth_factor,
											 double p_shrink_factor = default_shrink_factor, std::size_t p_adjustment_interval = default_adjustment_interval)
				: m_base_capacity(p_base_capacity), m_current_capacity(p_base_capacity), m_min_capacity(p_min_capacity), m_max_capacity(p_max_capacity), m_growth_factor(p_growth_factor),
				  m_shrink_factor(p_shrink_factor), m_adjustment_interval(p_adjustment_interval)
			{
			}

			// Destructor
			~dynamic_capacity_policy() override = default;

			// Deleted copy constructor and assignment operator
			dynamic_capacity_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			dynamic_capacity_policy(self_t&& p_other) noexcept
				: m_base_capacity(p_other.m_base_capacity), m_current_capacity(p_other.m_current_capacity), m_min_capacity(p_other.m_min_capacity), m_max_capacity(p_other.m_max_capacity),
				  m_growth_factor(p_other.m_growth_factor), m_shrink_factor(p_other.m_shrink_factor), m_adjustment_counter(p_other.m_adjustment_counter),
				  m_adjustment_interval(p_other.m_adjustment_interval)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_base_capacity		  = p_other.m_base_capacity;
					m_current_capacity	  = p_other.m_current_capacity;
					m_min_capacity		  = p_other.m_min_capacity;
					m_max_capacity		  = p_other.m_max_capacity;
					m_growth_factor		  = p_other.m_growth_factor;
					m_shrink_factor		  = p_other.m_shrink_factor;
					m_adjustment_counter  = p_other.m_adjustment_counter;
					m_adjustment_interval = p_other.m_adjustment_interval;
				}
				return *this;
			}

			auto capacity() const -> std::size_t override { return m_current_capacity; }

			auto set_capacity(std::size_t p_new_capacity) -> void override
			{
				m_base_capacity	   = p_new_capacity;
				m_current_capacity = std::max(m_min_capacity, std::min(p_new_capacity, m_max_capacity));
			}

			auto needs_eviction(std::size_t p_current_size) const -> bool override { return p_current_size >= m_current_capacity; }

			auto eviction_count(std::size_t p_current_size) const -> std::size_t override
			{
				if (p_current_size >= m_current_capacity)
				{
					return (p_current_size - m_current_capacity) + 1; // +1 for the new item
				}
				return 0;
			}

			/**
			 * @brief Manually trigger capacity adjustment consideration
			 * @param p_current_size Current number of cached items
			 */
			auto consider_capacity_adjustment(std::size_t p_current_size) -> void
			{
				++m_adjustment_counter;

				if (m_adjustment_counter >= m_adjustment_interval)
				{
					m_adjustment_counter = 0;

					// Simple heuristic: if we're consistently near capacity, grow
					// If we're consistently well below capacity, shrink
					const double utilization = static_cast<double>(p_current_size) / static_cast<double>(m_current_capacity);

					if (utilization > high_utilization_threshold && m_current_capacity < m_max_capacity)
					{
						// High utilization: grow capacity
						const auto new_capacity = static_cast<std::size_t>(static_cast<double>(m_current_capacity) * m_growth_factor);
						m_current_capacity		= std::min(new_capacity, m_max_capacity);
					}
					else if (utilization < low_utilization_threshold && m_current_capacity > m_min_capacity)
					{
						// Low utilization: shrink capacity
						const auto new_capacity = static_cast<std::size_t>(static_cast<double>(m_current_capacity) * m_shrink_factor);
						m_current_capacity		= std::max(new_capacity, m_min_capacity);
						m_current_capacity		= std::max(m_current_capacity, p_current_size); // Don't shrink below current size
					}
				}
			}
			/**
			 * @brief Configure growth parameters
			 * @param p_growth_factor The factor by which to grow capacity
			 * @param p_shrink_factor The factor by which to shrink capacity
			 */
			auto set_growth_parameters(double p_growth_factor, double p_shrink_factor) -> void
			{
				m_growth_factor = std::max(1.0, p_growth_factor);
				m_shrink_factor = std::max(min_shrink_factor, std::min(p_shrink_factor, 1.0));
			}

			/**
			 * @brief Set capacity bounds
			 * @param p_min_capacity The minimum allowed capacity
			 * @param p_max_capacity The maximum allowed capacity
			 */
			auto set_capacity_bounds(std::size_t p_min_capacity, std::size_t p_max_capacity) -> void
			{
				m_min_capacity	   = std::max(std::size_t{1}, p_min_capacity);
				m_max_capacity	   = std::max(m_min_capacity, p_max_capacity);
				m_current_capacity = std::max(m_min_capacity, std::min(m_current_capacity, m_max_capacity));
			}

			/**
			 * @brief Get the base capacity
			 * @return The base capacity
			 */
			auto base_capacity() const -> std::size_t { return m_base_capacity; }

			/**
			 * @brief Get the minimum capacity
			 * @return The minimum capacity
			 */
			auto min_capacity() const -> std::size_t { return m_min_capacity; }

			/**
			 * @brief Get the maximum capacity
			 * @return The maximum capacity
			 */
			auto max_capacity() const -> std::size_t { return m_max_capacity; }
		};

		/**
		 * @brief Soft capacity policy with gradual eviction
		 *
		 * Allows temporary capacity overruns but gradually evicts items
		 * to return to the target capacity. Useful for handling burst traffic.
		 */
		template <typename key_t, typename value_t> class soft_capacity_policy : public capacity_policy_base<key_t, value_t>
		{
		  public:
			using self_t = soft_capacity_policy<key_t, value_t>;
			using base_t = capacity_policy_base<key_t, value_t>;

		  private:
			static constexpr std::size_t default_target_capacity = 100;
			static constexpr double default_overage_tolerance	 = 0.2;

			std::size_t m_target_capacity;
			std::size_t m_max_capacity;
			double m_overage_tolerance;

		  public:
			// Constructor
			explicit soft_capacity_policy(std::size_t p_target_capacity = default_target_capacity,
										  double p_overage_tolerance	= default_overage_tolerance // Allow 20% overage
										  )
				: m_target_capacity(p_target_capacity), m_max_capacity(static_cast<std::size_t>(static_cast<double>(p_target_capacity) * (1.0 + p_overage_tolerance))),
				  m_overage_tolerance(p_overage_tolerance)
			{
			}

			// Destructor
			~soft_capacity_policy() override = default;

			// Copy constructor and assignment operator (deleted)
			soft_capacity_policy(const self_t&)		 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			soft_capacity_policy(self_t&& p_other) noexcept
				: m_target_capacity(p_other.m_target_capacity), m_max_capacity(p_other.m_max_capacity), m_overage_tolerance(p_other.m_overage_tolerance)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_target_capacity	= p_other.m_target_capacity;
					m_max_capacity		= p_other.m_max_capacity;
					m_overage_tolerance = p_other.m_overage_tolerance;
				}
				return *this;
			}

			auto capacity() const -> std::size_t override { return m_target_capacity; }

			auto set_capacity(std::size_t p_new_capacity) -> void override
			{
				m_target_capacity = p_new_capacity;
				m_max_capacity	  = static_cast<std::size_t>(static_cast<double>(p_new_capacity) * (1.0 + m_overage_tolerance));
			}

			auto needs_eviction(std::size_t p_current_size) const -> bool override
			{
				// Only require eviction when we exceed the hard maximum
				return p_current_size >= m_max_capacity;
			}

			auto eviction_count(std::size_t p_current_size) const -> std::size_t override
			{
				if (p_current_size >= m_max_capacity)
				{
					// Evict enough to get back to target capacity
					return (p_current_size - m_target_capacity) + 1; // +1 for the new item
				}
				if (p_current_size > m_target_capacity)
				{
					// Gradual eviction: evict one item to move toward target
					return 1;
				}
				return 0;
			}

			/**
			 * @brief Set the overage tolerance
			 * @param p_tolerance The tolerance as a fraction (0.2 = 20%)
			 */
			auto set_overage_tolerance(double p_tolerance) -> void
			{
				m_overage_tolerance = std::max(0.0, std::min(p_tolerance, 1.0));
				m_max_capacity		= static_cast<std::size_t>(static_cast<double>(m_target_capacity) * (1.0 + m_overage_tolerance));
			}

			/**
			 * @brief Get the current overage tolerance
			 * @return The tolerance as a fraction
			 */
			auto overage_tolerance() const -> double { return m_overage_tolerance; }

			/**
			 * @brief Get the maximum allowed capacity
			 * @return The maximum capacity
			 */
			auto max_capacity() const -> std::size_t { return m_max_capacity; }

			/**
			 * @brief Check if the cache is over its target capacity
			 * @param p_current_size The current cache size
			 * @return true if over target capacity
			 */
			auto is_over_target(std::size_t p_current_size) const -> bool { return p_current_size > m_target_capacity; }
		};

		/**
		 * @brief Memory-based capacity policy
		 *
		 * Manages capacity based on memory usage rather than item count.
		 * Useful when cache items have significantly different sizes.
		 */
		template <typename key_t, typename value_t> class memory_capacity_policy : public capacity_policy_base<key_t, value_t>
		{
		  public:
			using self_t = memory_capacity_policy<key_t, value_t>;
			using base_t = capacity_policy_base<key_t, value_t>;

		  private:
			static constexpr std::size_t default_memory_limit = static_cast<std::size_t>(1024) * 1024; // 1MB

			std::size_t m_memory_limit;
			mutable std::size_t m_current_memory_usage{0};
			std::size_t m_item_size_estimate;

		  public:
			// Constructor
			explicit memory_capacity_policy(std::size_t p_memory_limit = default_memory_limit, std::size_t p_item_size_estimate = sizeof(key_t) + sizeof(value_t))
				: m_memory_limit(p_memory_limit), m_item_size_estimate(p_item_size_estimate)
			{
			}

			// Destructor
			~memory_capacity_policy() override = default;

			// Copy constructor and assignment operator (deleted)
			memory_capacity_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			memory_capacity_policy(self_t&& p_other) noexcept
				: m_memory_limit(p_other.m_memory_limit), m_current_memory_usage(p_other.m_current_memory_usage), m_item_size_estimate(p_other.m_item_size_estimate)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_memory_limit		   = p_other.m_memory_limit;
					m_current_memory_usage = p_other.m_current_memory_usage;
					m_item_size_estimate   = p_other.m_item_size_estimate;
				}
				return *this;
			}

			auto capacity() const -> std::size_t override
			{
				// Return approximate item capacity based on memory limit
				return (m_item_size_estimate > 0) ? m_memory_limit / m_item_size_estimate : 0;
			}

			auto set_capacity(std::size_t p_new_capacity) -> void override
			{
				// Convert item capacity to memory limit
				m_memory_limit = p_new_capacity * m_item_size_estimate;
			}

			auto needs_eviction(std::size_t p_current_size) const -> bool override
			{
				// Update memory usage estimate
				m_current_memory_usage = p_current_size * m_item_size_estimate;

				return m_current_memory_usage >= m_memory_limit;
			}

			auto eviction_count(std::size_t p_current_size) const -> std::size_t override
			{
				const std::size_t estimated_usage = p_current_size * m_item_size_estimate;

				if (estimated_usage >= m_memory_limit)
				{
					const std::size_t excess_memory = estimated_usage - m_memory_limit + m_item_size_estimate; // +1 for new item
					return (excess_memory + m_item_size_estimate - 1) / m_item_size_estimate;				   // Ceiling division
				}
				return 0;
			}

			/**
			 * @brief Set the memory limit
			 * @param p_memory_limit The new memory limit in bytes
			 */
			auto set_memory_limit(std::size_t p_memory_limit) -> void { m_memory_limit = p_memory_limit; }

			/**
			 * @brief Get the current memory limit
			 * @return The memory limit in bytes
			 */
			auto memory_limit() const -> std::size_t { return m_memory_limit; }

			/**
			 * @brief Set the estimated size per item
			 * @param p_item_size The estimated size in bytes
			 */
			auto set_item_size_estimate(std::size_t p_item_size) -> void { m_item_size_estimate = std::max(std::size_t{1}, p_item_size); }

			/**
			 * @brief Get the estimated size per item
			 * @return The estimated size in bytes
			 */
			auto item_size_estimate() const -> std::size_t { return m_item_size_estimate; }

			/**
			 * @brief Get the current estimated memory usage
			 * @return The estimated memory usage in bytes
			 */
			auto current_memory_usage() const -> std::size_t { return m_current_memory_usage; }
		};

	} // namespace policies
} // namespace cache_engine