// File: inc/cache_engine/policies/eviction/random_policy.hpp

#pragma once

#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace cache_engine
{
	namespace policies
	{
		namespace eviction
		{
			template <typename key_t, typename value_t> class random_policy
			{
			  public:
				using self_t	 = random_policy<key_t, value_t>;
				using key_type	 = key_t;
				using value_type = value_t;
				using index_t	 = std::size_t;

			  private:
				std::vector<key_t> m_keys;
				std::unordered_map<key_t, index_t> m_key_to_index;
				bool m_seeded;

			  public:
				~random_policy() = default;

				random_policy() : m_seeded(false) {}

				random_policy(const self_t&)			 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				random_policy(self_t&& p_other) noexcept : m_keys(std::move(p_other.m_keys)), m_key_to_index(std::move(p_other.m_key_to_index)), m_seeded(p_other.m_seeded) {}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_keys		   = std::move(p_other.m_keys);
						m_key_to_index = std::move(p_other.m_key_to_index);
						m_seeded	   = p_other.m_seeded;
					}
					return *this;
				}

			  private:
				auto ensure_seeded() -> void
				{
					if (!m_seeded)
					{
						std::srand(static_cast<unsigned int>(std::time(nullptr)));
						m_seeded = true;
					}
				}

				auto random_index() -> index_t
				{
					ensure_seeded();
					return static_cast<index_t>(std::rand()) % m_keys.size();
				}

			  public:
				auto on_access(const key_t& p_key) -> void
				{
					// Random policy doesn't change order on access
					(void)p_key; // Suppress unused parameter warning
				}

				auto on_insert(const key_t& p_key) -> void
				{
					if (m_key_to_index.find(p_key) == m_key_to_index.end())
					{
						const index_t new_index = m_keys.size();
						m_keys.push_back(p_key);
						m_key_to_index[p_key] = new_index;
					}
				}

				auto select_victim() -> key_t
				{
					if (m_keys.empty())
					{
						throw std::runtime_error("Cannot select victim from empty cache");
					}

					const index_t victim_index = random_index();
					return m_keys[victim_index];
				}

				auto remove_key(const key_t& p_key) -> void
				{
					auto iter = m_key_to_index.find(p_key);
					if (iter != m_key_to_index.end())
					{
						const index_t key_index	 = iter->second;
						const index_t last_index = m_keys.size() - 1;

						if (key_index != last_index)
						{
							// Swap with last element (O(1) operation)
							const key_t& last_key	 = m_keys[last_index];
							m_keys[key_index]		 = last_key;
							m_key_to_index[last_key] = key_index;
						}

						// Remove last element (O(1) operation)
						m_keys.pop_back();
						m_key_to_index.erase(iter);
					}
				}

				auto size() const -> std::size_t { return m_keys.size(); }

				auto empty() const -> bool { return m_keys.empty(); }

				auto clear() -> void
				{
					m_keys.clear();
					m_key_to_index.clear();
				}

				auto seed_random(unsigned int p_seed) -> void
				{
					std::srand(p_seed);
					m_seeded = true;
				}
			};
		} // namespace eviction
	} // namespace policies
} // namespace cache_engine