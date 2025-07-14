// File: inc/cache_engine/policies/eviction/fifo_policy.hpp

#pragma once

#include <queue>
#include <stdexcept>
#include <unordered_set>

namespace cache_engine
{
	namespace policies
	{
		namespace eviction
		{
			template <typename key_t, typename value_t> class fifo_policy
			{
			  public:
				using self_t	 = fifo_policy<key_t, value_t>;
				using key_type	 = key_t;
				using value_type = value_t;

			  private:
				std::queue<key_t> m_insertion_queue;
				std::unordered_set<key_t> m_key_set;

			  public:
				~fifo_policy() = default;

				fifo_policy() = default;

				fifo_policy(const self_t&)				 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				fifo_policy(self_t&& p_other) noexcept : m_insertion_queue(std::move(p_other.m_insertion_queue)), m_key_set(std::move(p_other.m_key_set)) {}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_insertion_queue = std::move(p_other.m_insertion_queue);
						m_key_set		  = std::move(p_other.m_key_set);
					}
					return *this;
				}

			  public:
				auto on_access(const key_t& p_key) -> void
				{
					// FIFO doesn't change order on access
					(void)p_key; // Suppress unused parameter warning
				}

				auto on_insert(const key_t& p_key) -> void
				{
					if (m_key_set.find(p_key) == m_key_set.end())
					{
						m_insertion_queue.push(p_key);
						m_key_set.insert(p_key);
					}
				}

				auto select_victim() -> key_t
				{
					if (m_insertion_queue.empty())
					{
						throw std::runtime_error("Cannot select victim from empty cache");
					}
					return m_insertion_queue.front();
				}

				auto remove_key(const key_t& p_key) -> void
				{
					m_key_set.erase(p_key);
					// Note: We cannot efficiently remove from middle of std::queue
					// The victim removal is handled by select_victim() + queue.pop()
				}

				auto remove_victim() -> void
				{
					if (!m_insertion_queue.empty())
					{
						const key_t victim_key = m_insertion_queue.front();
						m_insertion_queue.pop();
						m_key_set.erase(victim_key);
					}
				}

				auto size() const -> std::size_t { return m_insertion_queue.size(); }

				auto empty() const -> bool { return m_insertion_queue.empty(); }

				auto clear() -> void
				{
					while (!m_insertion_queue.empty())
					{
						m_insertion_queue.pop();
					}
					m_key_set.clear();
				}
			};
		} // namespace eviction
	} // namespace policies
} // namespace cache_engine