/**
 * @file algorithm_comparison.cpp
 * @brief Head-to-head performance comparison between all policy-based cache algorithms
 * 
 * This file implements comparative benchmarks that directly compare all policy-based cache algorithms
 * under identical conditions to identify the fastest algorithm for different scenarios.
 */

#include <benchmark/benchmark.h>
#include <cache_engine/cache.hpp>
#include <cache_engine/policies/all_policies.hpp>
#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_set>

namespace cache_comparison
{
	// Policy-based cache type aliases for easier usage
	template <typename key_t, typename value_t>
	using lru_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::lru_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::update_on_access,
		cache_engine::policy_templates::fixed_capacity>;
	
	template <typename key_t, typename value_t>
	using fifo_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::fifo_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::no_update_on_access,
		cache_engine::policy_templates::fixed_capacity>;
	
	template <typename key_t, typename value_t>
	using lfu_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::lfu_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::update_on_access,
		cache_engine::policy_templates::fixed_capacity>;
	
	template <typename key_t, typename value_t>
	using mfu_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::mfu_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::update_on_access,
		cache_engine::policy_templates::fixed_capacity>;
	
	template <typename key_t, typename value_t>
	using mru_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::mru_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::update_on_access,
		cache_engine::policy_templates::fixed_capacity>;
	
	template <typename key_t, typename value_t>
	using random_cache = cache_engine::policy_based_cache<key_t, value_t, 
		cache_engine::policy_templates::random_eviction,
		cache_engine::policy_templates::hash_storage,
		cache_engine::policy_templates::no_update_on_access,
		cache_engine::policy_templates::fixed_capacity>;

	/**
	 * @brief Test scenario for algorithm comparison
	 */
	struct comparison_scenario
	{
		std::size_t cache_size;
		std::size_t key_range;
		std::size_t operations;
		double hit_ratio;
		std::string scenario_name;
	};

	/**
	 * @brief Workload patterns for testing different use cases
	 */
	enum class workload_pattern
	{
		mixed_operations,	// 70% get, 30% put
		read_heavy,			// 90% get, 10% put
		write_heavy,		// 30% get, 70% put
		sequential_access,	// Sequential key access pattern
		random_access		// Random key access pattern
	};

	/**
	 * @brief Workload generator for different access patterns
	 */
	class workload_generator
	{
	public:
		using key_t = std::int32_t;

	private:
		mutable std::mt19937 m_rng;
		std::size_t m_key_range;
		workload_pattern m_pattern;
		mutable std::size_t m_sequential_counter;

	public:
		explicit workload_generator(std::size_t p_key_range, workload_pattern p_pattern)
			: m_rng(42), m_key_range(p_key_range), m_pattern(p_pattern), m_sequential_counter(0)
		{
		}

		auto generate_operation() const -> std::pair<bool, key_t>  // true = get, false = put
		{
			bool is_get_operation = false;
			std::uniform_real_distribution<double> op_dist(0.0, 1.0);
			
			switch (m_pattern)
			{
				case workload_pattern::mixed_operations:
					is_get_operation = op_dist(m_rng) < 0.7;
					break;
				case workload_pattern::read_heavy:
					is_get_operation = op_dist(m_rng) < 0.9;
					break;
				case workload_pattern::write_heavy:
					is_get_operation = op_dist(m_rng) < 0.3;
					break;
				case workload_pattern::sequential_access:
				case workload_pattern::random_access:
					is_get_operation = op_dist(m_rng) < 0.7;  // Default mixed
					break;
			}

			key_t key = 0;
			if (m_pattern == workload_pattern::sequential_access)
			{
				key = static_cast<key_t>(m_sequential_counter % m_key_range);
				++m_sequential_counter;
			}
			else
			{
				std::uniform_int_distribution<key_t> key_dist(0, static_cast<key_t>(m_key_range - 1));
				key = key_dist(m_rng);
			}

			return std::make_pair(is_get_operation, key);
		}

		auto generate_workload(std::size_t p_operation_count) const -> std::vector<std::pair<bool, key_t>>
		{
			std::vector<std::pair<bool, key_t>> workload;
			workload.reserve(p_operation_count);
			
			for (std::size_t idx_for = 0; idx_for < p_operation_count; ++idx_for)
			{
				workload.push_back(generate_operation());
			}
			
			return workload;
		}
	};

	/**
	 * @brief Benchmark template for any policy-based cache algorithm
	 */
	template<template<typename, typename> class cache_template>
	auto benchmark_algorithm_impl(benchmark::State& p_state, const comparison_scenario& p_scenario, workload_pattern p_pattern) -> void
	{
		using cache_t = cache_template<std::int32_t, std::string>;
		
		// Initialize cache
		cache_t cache(p_scenario.cache_size);
		
		// Generate workload
		workload_generator workload_gen(p_scenario.key_range, p_pattern);
		const auto operations = workload_gen.generate_workload(p_scenario.operations);
		
		// Pre-populate cache to achieve desired hit ratio
		const std::size_t populate_count = static_cast<std::size_t>(static_cast<double>(p_scenario.cache_size) * p_scenario.hit_ratio);
		for (std::size_t idx_for = 0; idx_for < populate_count; ++idx_for)
		{
			const auto key = static_cast<std::int32_t>(idx_for % p_scenario.key_range);
			cache.put(key, "value_" + std::to_string(key));
		}

		// Benchmark execution
		std::size_t operation_count = 0;
		std::size_t hit_count = 0;
		std::size_t miss_count = 0;

		for (auto _ : p_state)
		{
			const auto& operation = operations[operation_count % operations.size()];
			const bool is_get = operation.first;
			const auto key = operation.second;
			
			if (is_get)
			{
				try
				{
					benchmark::DoNotOptimize(cache.get(key));
					++hit_count;
				}
				catch (const std::out_of_range&)
				{
					++miss_count;
				}
			}
			else
			{
				cache.put(key, "value_" + std::to_string(key));
			}
			
			++operation_count;
		}

		// Report metrics
		p_state.SetItemsProcessed(static_cast<std::int64_t>(operation_count));
		p_state.SetBytesProcessed(static_cast<std::int64_t>(operation_count * (sizeof(std::int32_t) + 20)));
		
		// Calculate and report hit rate
		if (hit_count + miss_count > 0)
		{
			const double actual_hit_rate = static_cast<double>(hit_count) / static_cast<double>(hit_count + miss_count);
			p_state.counters["HitRate"] = benchmark::Counter(actual_hit_rate, benchmark::Counter::kAvgThreads);
		}
		p_state.counters["Hits"] = benchmark::Counter(static_cast<double>(hit_count), benchmark::Counter::kAvgThreads);
		p_state.counters["Misses"] = benchmark::Counter(static_cast<double>(miss_count), benchmark::Counter::kAvgThreads);
	}

	// Define common scenarios
	const comparison_scenario small_mixed = {100, 500, 10000, 0.8, "SmallMixed"};
	const comparison_scenario medium_mixed = {1000, 5000, 100000, 0.8, "MediumMixed"};
	const comparison_scenario large_mixed = {10000, 50000, 1000000, 0.8, "LargeMixed"};
	const comparison_scenario low_hit_rate = {1000, 10000, 100000, 0.1, "LowHitRate"};
	const comparison_scenario high_hit_rate = {1000, 2000, 100000, 0.95, "HighHitRate"};

	// LRU Algorithm Benchmarks
	auto benchmark_lru_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lru_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lru_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lru_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_lru_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_lru_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_lru_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_lru_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lru_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

	// FIFO Algorithm Benchmarks
	auto benchmark_fifo_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_fifo_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_fifo_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_fifo_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_fifo_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_fifo_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_fifo_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_fifo_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<fifo_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

	// LFU Algorithm Benchmarks
	auto benchmark_lfu_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lfu_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lfu_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_lfu_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_lfu_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_lfu_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_lfu_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_lfu_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<lfu_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

	// MFU Algorithm Benchmarks
	auto benchmark_mfu_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mfu_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mfu_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mfu_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_mfu_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_mfu_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_mfu_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_mfu_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mfu_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

	// MRU Algorithm Benchmarks
	auto benchmark_mru_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mru_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mru_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_mru_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_mru_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_mru_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_mru_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_mru_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<mru_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

	// Random Algorithm Benchmarks
	auto benchmark_random_small_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, small_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_random_medium_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, medium_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_random_large_mixed(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, large_mixed, workload_pattern::mixed_operations);
	}

	auto benchmark_random_read_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, medium_mixed, workload_pattern::read_heavy);
	}

	auto benchmark_random_write_heavy(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, medium_mixed, workload_pattern::write_heavy);
	}

	auto benchmark_random_sequential(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, medium_mixed, workload_pattern::sequential_access);
	}

	auto benchmark_random_low_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, low_hit_rate, workload_pattern::mixed_operations);
	}

	auto benchmark_random_high_hit_rate(benchmark::State& p_state) -> void
	{
		benchmark_algorithm_impl<random_cache>(p_state, high_hit_rate, workload_pattern::mixed_operations);
	}

}	// namespace cache_comparison

// Register LRU benchmarks
BENCHMARK(cache_comparison::benchmark_lru_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lru_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register FIFO benchmarks
BENCHMARK(cache_comparison::benchmark_fifo_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_fifo_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register LFU benchmarks
BENCHMARK(cache_comparison::benchmark_lfu_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_lfu_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MFU benchmarks
BENCHMARK(cache_comparison::benchmark_mfu_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mfu_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MRU benchmarks
BENCHMARK(cache_comparison::benchmark_mru_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_mru_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register Random benchmarks
BENCHMARK(cache_comparison::benchmark_random_small_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_medium_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_large_mixed)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_read_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_write_heavy)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_sequential)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_low_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_comparison::benchmark_random_high_hit_rate)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();