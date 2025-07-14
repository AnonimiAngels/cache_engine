// File: inc/cache_engine/policies/eviction/mfu_policy.hpp

#pragma once

#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace cache_engine
{
	namespace policies
	{
		namespace eviction
		{
			template <typename key_t, typename value_t> class mfu_policy
			{
			  public:
				using self_t	  = mfu_policy<key_t, value_t>;
				using key_type	  = key_t;
				using value_type  = value_t;
				using frequency_t = std::size_t;

			  private:
				std::unordered_map<key_t, frequency_t> m_key_to_frequency;
				std::map<frequency_t, std::list<key_t>> m_frequency_to_keys;

			  public:
				~mfu_policy() = default;

				mfu_policy() = default;

				mfu_policy(const self_t&)				 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				mfu_policy(self_t&& p_other) noexcept : m_key_to_frequency(std::move(p_other.m_key_to_frequency)), m_frequency_to_keys(std::move(p_other.m_frequency_to_keys)) {}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_key_to_frequency	= std::move(p_other.m_key_to_frequency);
						m_frequency_to_keys = std::move(p_other.m_frequency_to_keys);
					}
					return *this;
				}

			  private:
				auto update_frequency(const key_t& p_key) -> void
				{
					auto freq_iter = m_key_to_frequency.find(p_key);
					if (freq_iter != m_key_to_frequency.end())
					{
						const frequency_t old_freq = freq_iter->second;
						const frequency_t new_freq = old_freq + 1;

						// Remove key from old frequency list
						auto& old_freq_list = m_frequency_to_keys[old_freq];
						old_freq_list.remove(p_key);
						if (old_freq_list.empty())
						{
							m_frequency_to_keys.erase(old_freq);
						}

						// Add key to new frequency list
						m_frequency_to_keys[new_freq].push_back(p_key);
						freq_iter->second = new_freq;
					}
				}

			  public:
				auto on_access(const key_t& p_key) -> void { update_frequency(p_key); }

				auto on_insert(const key_t& p_key) -> void
				{
					const frequency_t initial_freq = 1;
					m_key_to_frequency[p_key]	   = initial_freq;
					m_frequency_to_keys[initial_freq].push_back(p_key);
				}

				auto select_victim() -> key_t
				{
					if (m_frequency_to_keys.empty())
					{
						throw std::runtime_error("Cannot select victim from empty cache");
					}

					// Select most frequently used (highest frequency)
					auto& most_freq_list = m_frequency_to_keys.rbegin()->second;
					if (most_freq_list.empty())
					{
						throw std::runtime_error("Frequency list is empty");
					}

					return most_freq_list.front();
				}

				auto remove_key(const key_t& p_key) -> void
				{
					auto freq_iter = m_key_to_frequency.find(p_key);
					if (freq_iter != m_key_to_frequency.end())
					{
						const frequency_t freq = freq_iter->second;

						// Remove from frequency list
						auto& freq_list = m_frequency_to_keys[freq];
						freq_list.remove(p_key);
						if (freq_list.empty())
						{
							m_frequency_to_keys.erase(freq);
						}

						// Remove from frequency map
						m_key_to_frequency.erase(freq_iter);
					}
				}

				auto size() const -> std::size_t { return m_key_to_frequency.size(); }

				auto empty() const -> bool { return m_key_to_frequency.empty(); }

				auto clear() -> void
				{
					m_key_to_frequency.clear();
					m_frequency_to_keys.clear();
				}

				auto get_frequency(const key_t& p_key) const -> frequency_t
				{
					auto iter = m_key_to_frequency.find(p_key);
					return (iter != m_key_to_frequency.end()) ? iter->second : 0;
				}
			};
		} // namespace eviction
	} // namespace policies
} // namespace cache_engine