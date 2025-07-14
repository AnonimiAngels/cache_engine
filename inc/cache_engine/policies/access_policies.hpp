#pragma once

#include "policy_interfaces.hpp"

namespace cache_engine
{
	namespace policies
	{
		/**
		 * @brief Standard access policy that updates eviction order on access
		 *
		 * This is the typical behavior for most cache algorithms where
		 * accessing a key affects its position in the eviction order.
		 */
		template <typename key_t, typename value_t> class update_on_access_policy : public access_policy_base<key_t, value_t>
		{
		  public:
			using self_t = update_on_access_policy<key_t, value_t>;
			using base_t = access_policy_base<key_t, value_t>;

			// Constructor
			update_on_access_policy() = default;

			// Destructor
			~update_on_access_policy() override = default;

			// Deleted copy constructor and assignment operator
			update_on_access_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			update_on_access_policy(self_t&& p_other) noexcept	 = default;
			auto operator=(self_t&& p_other) noexcept -> self_t& = default;
			auto on_access(const key_t& p_key, eviction_policy_base<key_t, value_t>& p_eviction_policy) -> bool override
			{
				// Always update eviction order on access
				static_cast<void>(p_key);
				static_cast<void>(p_eviction_policy);
				return true;
			}

			auto on_miss(const key_t& p_key) -> bool override
			{
				// Record cache misses (default behavior)
				static_cast<void>(p_key);
				return true;
			}
		};

		/**
		 * @brief Access policy that does not update eviction order on access
		 *
		 * Useful for scenarios where you want to observe cache behavior
		 * without affecting the eviction order. Good for read-only analysis
		 * or when implementing specific cache semantics.
		 */
		template <typename key_t, typename value_t> class no_update_on_access_policy : public access_policy_base<key_t, value_t>
		{
		  public:
			using self_t = no_update_on_access_policy<key_t, value_t>;
			using base_t = access_policy_base<key_t, value_t>;

			// Constructor
			no_update_on_access_policy() = default;

			// Destructor
			~no_update_on_access_policy() override = default;

			// Deleted copy constructor and assignment operator
			no_update_on_access_policy(const self_t&) = delete;
			auto operator=(const self_t&) -> self_t&  = delete;

			// Move constructor and assignment operator
			no_update_on_access_policy(self_t&& p_other) noexcept = default;
			auto operator=(self_t&& p_other) noexcept -> self_t&  = default;
			auto on_access(const key_t& p_key, eviction_policy_base<key_t, value_t>& p_eviction_policy) -> bool override
			{
				// Never update eviction order on access
				static_cast<void>(p_key);
				static_cast<void>(p_eviction_policy);
				return false;
			}

			auto on_miss(const key_t& p_key) -> bool override
			{
				// Still record cache misses
				static_cast<void>(p_key);
				return true;
			}
		};

		/**
		 * @brief Conditional access policy that updates based on access frequency
		 *
		 * Only updates eviction order if a key has been accessed more than
		 * a specified threshold number of times. Useful for avoiding
		 * cache pollution from one-time accesses.
		 */
		template <typename key_t, typename value_t> class threshold_access_policy : public access_policy_base<key_t, value_t>
		{
		  public:
			using self_t = threshold_access_policy<key_t, value_t>;
			using base_t = access_policy_base<key_t, value_t>;

		  private:
			std::unordered_map<key_t, std::size_t> m_access_counts;
			std::size_t m_threshold;

		  public:
			// Constructor with threshold
			explicit threshold_access_policy(std::size_t p_threshold = 2) : m_threshold(p_threshold) {}

			// Destructor
			~threshold_access_policy() override = default;

			// Deleted copy constructor and assignment operator
			threshold_access_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			threshold_access_policy(self_t&& p_other) noexcept : m_access_counts(std::move(p_other.m_access_counts)), m_threshold(p_other.m_threshold) {}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_access_counts = std::move(p_other.m_access_counts);
					m_threshold		= p_other.m_threshold;
				}
				return *this;
			}

			auto on_access(const key_t& p_key, eviction_policy_base<key_t, value_t>& p_eviction_policy) -> bool override
			{
				// Increment access count
				++m_access_counts[p_key];

				// Only update eviction order if threshold is met
				static_cast<void>(p_eviction_policy);
				return m_access_counts[p_key] >= m_threshold;
			}

			auto on_miss(const key_t& p_key) -> bool override
			{
				// Record cache misses
				static_cast<void>(p_key);
				return true;
			}
			/**
			 * @brief Set the access threshold
			 * @param p_threshold The new threshold value
			 */
			auto set_threshold(std::size_t p_threshold) -> void { m_threshold = p_threshold; }

			/**
			 * @brief Get the current access threshold
			 * @return The threshold value
			 */
			auto threshold() const -> std::size_t { return m_threshold; }

			/**
			 * @brief Get the access count for a specific key
			 * @param p_key The key to query
			 * @return The number of times the key has been accessed
			 */
			auto access_count(const key_t& p_key) const -> std::size_t
			{
				auto iter = m_access_counts.find(p_key);
				return (iter != m_access_counts.end()) ? iter->second : 0;
			}

			/**
			 * @brief Clear all access counts
			 */
			auto clear_access_counts() -> void { m_access_counts.clear(); }
		};

		/**
		 * @brief Time-based access policy with decay
		 *
		 * Updates eviction order based on time intervals and applies
		 * decay to access frequencies over time. Useful for time-sensitive
		 * caching scenarios.
		 */
		template <typename key_t, typename value_t> class time_decay_access_policy : public access_policy_base<key_t, value_t>
		{
		  public:
			using self_t = time_decay_access_policy<key_t, value_t>;
			using base_t = access_policy_base<key_t, value_t>;

		  private:
			static constexpr std::size_t default_decay_interval = 100;

			std::unordered_map<key_t, std::size_t> m_last_access_time;
			std::size_t m_current_time{0};
			std::size_t m_decay_interval;

		  public:
			// Constructor with decay interval
			explicit time_decay_access_policy(std::size_t p_decay_interval = default_decay_interval) : m_decay_interval(p_decay_interval) {}

			// Destructor
			~time_decay_access_policy() override = default;

			// Deleted copy constructor and assignment operator
			time_decay_access_policy(const self_t&)	 = delete;
			auto operator=(const self_t&) -> self_t& = delete;

			// Move constructor and assignment operator
			time_decay_access_policy(self_t&& p_other) noexcept
				: m_last_access_time(std::move(p_other.m_last_access_time)), m_current_time(p_other.m_current_time), m_decay_interval(p_other.m_decay_interval)
			{
			}

			auto operator=(self_t&& p_other) noexcept -> self_t&
			{
				if (this != &p_other)
				{
					m_last_access_time = std::move(p_other.m_last_access_time);
					m_current_time	   = p_other.m_current_time;
					m_decay_interval   = p_other.m_decay_interval;
				}
				return *this;
			}

			auto on_access(const key_t& p_key, eviction_policy_base<key_t, value_t>& p_eviction_policy) -> bool override
			{
				++m_current_time;

				// Record access time
				m_last_access_time[p_key] = m_current_time;

				// Apply decay periodically
				if (m_current_time % m_decay_interval == 0)
				{
					this->apply_decay();
				}

				// Always update eviction order for time-based policy
				static_cast<void>(p_eviction_policy);
				return true;
			}

			auto on_miss(const key_t& p_key) -> bool override
			{
				++m_current_time;

				// Record cache misses
				static_cast<void>(p_key);
				return true;
			}
			/**
			 * @brief Set the decay interval
			 * @param p_interval The new decay interval
			 */
			auto set_decay_interval(std::size_t p_interval) -> void { m_decay_interval = (p_interval > 0) ? p_interval : 1; }

			/**
			 * @brief Get the current decay interval
			 * @return The decay interval
			 */
			auto decay_interval() const -> std::size_t { return m_decay_interval; }

			/**
			 * @brief Get the current logical time
			 * @return The current time counter
			 */
			auto current_time() const -> std::size_t { return m_current_time; }

			/**
			 * @brief Get the last access time for a specific key
			 * @param p_key The key to query
			 * @return The last access time (0 if never accessed)
			 */
			auto last_access_time(const key_t& p_key) const -> std::size_t
			{
				auto iter = m_last_access_time.find(p_key);
				return (iter != m_last_access_time.end()) ? iter->second : 0;
			}

		  private:
			auto apply_decay() -> void
			{
				// Remove entries that are too old (simple decay strategy)
				const std::size_t cutoff_time = (m_current_time > (m_decay_interval * 2)) ? (m_current_time - (m_decay_interval * 2)) : 0;

				auto iter = m_last_access_time.begin();
				while (iter != m_last_access_time.end())
				{
					if (iter->second < cutoff_time)
					{
						iter = m_last_access_time.erase(iter);
					}
					else
					{
						++iter;
					}
				}
			}
		};

	} // namespace policies
} // namespace cache_engine