// File: inc/cache_engine/policies/eviction/lru_policy.hpp

#pragma once

#include <list>
#include <stdexcept>
#include <unordered_map>

namespace cache_engine
{
	namespace policies
	{
		namespace eviction
		{
			template <typename key_t, typename value_t> class lru_policy
			{
			  public:
				using self_t		= lru_policy<key_t, value_t>;
				using key_type		= key_t;
				using value_type	= value_t;
				using list_iterator = typename std::list<key_t>::iterator;

			  private:
				std::list<key_t> m_access_list;
				std::unordered_map<key_t, list_iterator> m_key_to_iterator;

			  public:
				~lru_policy() = default;

				lru_policy() = default;

				lru_policy(const self_t&)				 = delete;
				auto operator=(const self_t&) -> self_t& = delete;

				lru_policy(self_t&& p_other) noexcept : m_access_list(std::move(p_other.m_access_list)), m_key_to_iterator(std::move(p_other.m_key_to_iterator)) {}

				auto operator=(self_t&& p_other) noexcept -> self_t&
				{
					if (this != &p_other)
					{
						m_access_list	  = std::move(p_other.m_access_list);
						m_key_to_iterator = std::move(p_other.m_key_to_iterator);
					}
					return *this;
				}

			  public:
				auto on_access(const key_t& p_key) -> void
				{
					auto iter_map = m_key_to_iterator.find(p_key);
					if (iter_map != m_key_to_iterator.end())
					{
						// Move key to front (most recently used)
						m_access_list.splice(m_access_list.begin(), m_access_list, iter_map->second);
					}
				}

				auto on_insert(const key_t& p_key) -> void
				{
					// Add key to front of list (most recently used)
					m_access_list.push_front(p_key);
					m_key_to_iterator[p_key] = m_access_list.begin();
				}

				auto select_victim() -> key_t
				{
					if (m_access_list.empty())
					{
						throw std::runtime_error("Cannot select victim from empty cache");
					}
					return m_access_list.back();
				}

				auto remove_key(const key_t& p_key) -> void
				{
					auto iter_map = m_key_to_iterator.find(p_key);
					if (iter_map != m_key_to_iterator.end())
					{
						m_access_list.erase(iter_map->second);
						m_key_to_iterator.erase(iter_map);
					}
				}

				auto size() const -> std::size_t { return m_access_list.size(); }

				auto empty() const -> bool { return m_access_list.empty(); }

				auto clear() -> void
				{
					m_access_list.clear();
					m_key_to_iterator.clear();
				}
			};
		} // namespace eviction
	} // namespace policies
} // namespace cache_engine