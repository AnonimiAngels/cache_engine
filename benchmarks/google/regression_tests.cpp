/**
 * @file regression_tests.cpp
 * @brief Performance regression detection benchmarks for cache algorithms
 * 
 * This file implements regression benchmarks that establish performance baselines
 * and detect performance regressions across different versions of the cache engine.
 * These benchmarks are designed to be stable and reproducible for CI/CD integration.
 */

#include <benchmark/benchmark.h>
#include <cache_engine/cache.hpp>
#include <cache_engine/policies/all_policies.hpp>
#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <numeric>

namespace cache_regression
{
	/**
	 * @brief Regression test configuration with expected performance bounds
	 */
	struct regression_config
	{
		std::size_t cache_size;
		std::size_t key_range;
		std::size_t operations;
		double expected_min_ops_per_sec;  // Minimum acceptable throughput
		double expected_max_latency_ns;   // Maximum acceptable latency
		std::string test_name;
	};

	/**
	 * @brief Deterministic workload generator for reproducible regression tests
	 */
	class deterministic_workload
	{
	public:
		using key_t = std::int32_t;

	private:
		std::mt19937 m_rng;
		std::size_t m_key_range;
		std::vector<std::pair<bool, key_t>> m_operations;

	public:
		explicit deterministic_workload(std::size_t p_key_range, std::size_t p_operation_count, std::uint32_t p_seed = 12345)
			: m_rng(p_seed), m_key_range(p_key_range)
		{
			generate_operations(p_operation_count);
		}

		auto get_operations() const -> const std::vector<std::pair<bool, key_t>>& 
		{
			return m_operations;
		}

	private:
		auto generate_operations(std::size_t p_count) -> void
		{
			m_operations.clear();
			m_operations.reserve(p_count);
			
			std::uniform_int_distribution<key_t> key_dist(0, static_cast<key_t>(m_key_range - 1));
			std::uniform_real_distribution<double> op_dist(0.0, 1.0);
			
			for (std::size_t idx_for = 0; idx_for < p_count; ++idx_for)
			{
				const bool is_get = op_dist(m_rng) < 0.75;  // 75% get, 25% put
				const auto key = key_dist(m_rng);
				m_operations.emplace_back(is_get, key);
			}
		}
	};

	/**
	 * @brief Baseline performance regression test
	 */
	template<typename algorithm_t>
	auto benchmark_baseline_performance(benchmark::State& p_state, const regression_config& p_config) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		// Create deterministic workload
		deterministic_workload workload(p_config.key_range, p_config.operations);
		const auto& operations = workload.get_operations();
		
		std::size_t total_operations = 0;
		std::size_t hit_count = 0;
		std::size_t miss_count = 0;
		
		for (auto _ : p_state)
		{
			cache_t cache(p_config.cache_size);
			
			// Pre-populate cache with predictable data
			for (std::size_t idx_for = 0; idx_for < p_config.cache_size / 2; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for);
				cache.put(key, "baseline_value_" + std::to_string(key));
			}
			
			// Execute deterministic workload
			for (const auto& operation : operations)
			{
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
					cache.put(key, "regression_value_" + std::to_string(key));
				}
			}
			
			total_operations += operations.size();
		}

		// Report regression metrics
		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.SetBytesProcessed(static_cast<std::int64_t>(total_operations * (sizeof(std::int32_t) + 25)));
		
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(p_config.cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["Operations"] = benchmark::Counter(static_cast<double>(p_config.operations), benchmark::Counter::kAvgThreads);
		
		if (hit_count + miss_count > 0)
		{
			const double hit_rate = static_cast<double>(hit_count) / static_cast<double>(hit_count + miss_count);
			p_state.counters["HitRate"] = benchmark::Counter(hit_rate, benchmark::Counter::kAvgThreads);
		}
		
		p_state.counters["Hits"] = benchmark::Counter(static_cast<double>(hit_count), benchmark::Counter::kAvgThreads);
		p_state.counters["Misses"] = benchmark::Counter(static_cast<double>(miss_count), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Sequential access pattern regression test
	 */
	template<typename algorithm_t>
	auto benchmark_sequential_regression(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = 1000;
		const std::size_t sequence_length = 5000;
		
		std::size_t total_operations = 0;
		
		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			// Sequential access pattern
			for (std::size_t idx_for = 0; idx_for < sequence_length; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for % (cache_size * 2));
				
				if ((idx_for % 4) == 0)  // Every 4th operation is a put
				{
					cache.put(key, "seq_value_" + std::to_string(key));
				}
				else  // Other operations are gets
				{
					try
					{
						benchmark::DoNotOptimize(cache.get(key));
					}
					catch (const std::out_of_range&)
					{
						// Sequential misses are expected
					}
				}
			}
			
			total_operations += sequence_length;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["SequenceLength"] = benchmark::Counter(static_cast<double>(sequence_length), benchmark::Counter::kAvgThreads);
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Hot/cold data access regression test
	 */
	template<typename algorithm_t>
	auto benchmark_hotcold_regression(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = 1000;
		const std::size_t hot_keys = cache_size / 10;  // 10% hot keys
		const std::size_t cold_keys = cache_size * 10; // 10x cold keys
		const std::size_t operations = 10000;
		
		std::mt19937 rng(54321);  // Fixed seed for reproducibility
		std::uniform_int_distribution<std::int32_t> hot_dist(0, static_cast<std::int32_t>(hot_keys - 1));
		std::uniform_int_distribution<std::int32_t> cold_dist(static_cast<std::int32_t>(hot_keys), static_cast<std::int32_t>(cold_keys - 1));
		std::uniform_real_distribution<double> access_dist(0.0, 1.0);
		
		std::size_t total_operations = 0;
		std::size_t hot_accesses = 0;
		std::size_t cold_accesses = 0;
		
		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			// Pre-populate with hot data
			for (std::size_t idx_for = 0; idx_for < hot_keys; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for);
				cache.put(key, "hot_value_" + std::to_string(key));
			}
			
			// Hot/cold access pattern: 80% hot, 20% cold
			for (std::size_t idx_for = 0; idx_for < operations; ++idx_for)
			{
				std::int32_t key = 0;
				if (access_dist(rng) < 0.8)  // 80% hot accesses
				{
					key = hot_dist(rng);
					++hot_accesses;
				}
				else  // 20% cold accesses
				{
					key = cold_dist(rng);
					++cold_accesses;
				}
				
				if ((idx_for % 5) == 0)  // 20% put operations
				{
					cache.put(key, "hotcold_value_" + std::to_string(key));
				}
				else  // 80% get operations
				{
					try
					{
						benchmark::DoNotOptimize(cache.get(key));
					}
					catch (const std::out_of_range&)
					{
						// Cold misses are expected
					}
				}
			}
			
			total_operations += operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["HotAccesses"] = benchmark::Counter(static_cast<double>(hot_accesses), benchmark::Counter::kAvgThreads);
		p_state.counters["ColdAccesses"] = benchmark::Counter(static_cast<double>(cold_accesses), benchmark::Counter::kAvgThreads);
		p_state.counters["HotRatio"] = benchmark::Counter(static_cast<double>(hot_accesses) / static_cast<double>(total_operations), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Capacity boundary regression test
	 */
	template<typename algorithm_t>
	auto benchmark_capacity_boundary(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = 500;
		const std::size_t boundary_operations = cache_size * 3;  // 3x capacity
		
		std::size_t total_operations = 0;
		std::size_t boundary_hits = 0;
		
		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			// Fill cache to exactly capacity
			for (std::size_t idx_for = 0; idx_for < cache_size; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for);
				cache.put(key, "boundary_value_" + std::to_string(key));
			}
			
			// Test boundary behavior with operations that exceed capacity
			for (std::size_t idx_for = 0; idx_for < boundary_operations; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for % (cache_size * 2));
				
				// Try to get first, then put if not found
				try
				{
					benchmark::DoNotOptimize(cache.get(key));
					++boundary_hits;
				}
				catch (const std::out_of_range&)
				{
					cache.put(key, "new_boundary_value_" + std::to_string(key));
				}
			}
			
			total_operations += boundary_operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["BoundaryHits"] = benchmark::Counter(static_cast<double>(boundary_hits), benchmark::Counter::kAvgThreads);
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["BoundaryOps"] = benchmark::Counter(static_cast<double>(boundary_operations), benchmark::Counter::kAvgThreads);
	}

	// Regression test configurations
	const regression_config small_regression = {100, 500, 5000, 50000.0, 1000.0, "SmallRegression"};
	const regression_config medium_regression = {1000, 5000, 50000, 100000.0, 500.0, "MediumRegression"};
	const regression_config large_regression = {10000, 50000, 500000, 150000.0, 300.0, "LargeRegression"};

	// Forward declarations for regression benchmark functions
	auto benchmark_lru_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_lru_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_lru_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_lru_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_lru_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_lru_capacity_boundary(benchmark::State& p_state) -> void;
	auto benchmark_fifo_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_fifo_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_fifo_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_fifo_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_fifo_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_fifo_capacity_boundary(benchmark::State& p_state) -> void;
	auto benchmark_lfu_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_lfu_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_lfu_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_lfu_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_lfu_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_lfu_capacity_boundary(benchmark::State& p_state) -> void;
	auto benchmark_mfu_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_mfu_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_mfu_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_mfu_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_mfu_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_mfu_capacity_boundary(benchmark::State& p_state) -> void;
	auto benchmark_mru_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_mru_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_mru_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_mru_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_mru_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_mru_capacity_boundary(benchmark::State& p_state) -> void;
	auto benchmark_random_small_regression(benchmark::State& p_state) -> void;
	auto benchmark_random_medium_regression(benchmark::State& p_state) -> void;
	auto benchmark_random_large_regression(benchmark::State& p_state) -> void;
	auto benchmark_random_sequential_regression(benchmark::State& p_state) -> void;
	auto benchmark_random_hotcold_regression(benchmark::State& p_state) -> void;
	auto benchmark_random_capacity_boundary(benchmark::State& p_state) -> void;

	// LRU Regression Tests
	auto benchmark_lru_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lru>(p_state, small_regression);
	}

	auto benchmark_lru_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lru>(p_state, medium_regression);
	}

	auto benchmark_lru_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lru>(p_state, large_regression);
	}

	auto benchmark_lru_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::lru>(p_state);
	}

	auto benchmark_lru_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::lru>(p_state);
	}

	auto benchmark_lru_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::lru>(p_state);
	}

	// FIFO Regression Tests
	auto benchmark_fifo_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::fifo>(p_state, small_regression);
	}

	auto benchmark_fifo_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::fifo>(p_state, medium_regression);
	}

	auto benchmark_fifo_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::fifo>(p_state, large_regression);
	}

	auto benchmark_fifo_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::fifo>(p_state);
	}

	auto benchmark_fifo_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::fifo>(p_state);
	}

	auto benchmark_fifo_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::fifo>(p_state);
	}

	// LFU Regression Tests
	auto benchmark_lfu_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lfu>(p_state, small_regression);
	}

	auto benchmark_lfu_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lfu>(p_state, medium_regression);
	}

	auto benchmark_lfu_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::lfu>(p_state, large_regression);
	}

	auto benchmark_lfu_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::lfu>(p_state);
	}

	auto benchmark_lfu_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::lfu>(p_state);
	}

	auto benchmark_lfu_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::lfu>(p_state);
	}

	// MFU Regression Tests
	auto benchmark_mfu_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mfu>(p_state, small_regression);
	}

	auto benchmark_mfu_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mfu>(p_state, medium_regression);
	}

	auto benchmark_mfu_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mfu>(p_state, large_regression);
	}

	auto benchmark_mfu_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::mfu>(p_state);
	}

	auto benchmark_mfu_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::mfu>(p_state);
	}

	auto benchmark_mfu_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::mfu>(p_state);
	}

	// MRU Regression Tests
	auto benchmark_mru_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mru>(p_state, small_regression);
	}

	auto benchmark_mru_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mru>(p_state, medium_regression);
	}

	auto benchmark_mru_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::mru>(p_state, large_regression);
	}

	auto benchmark_mru_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::mru>(p_state);
	}

	auto benchmark_mru_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::mru>(p_state);
	}

	auto benchmark_mru_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::mru>(p_state);
	}

	// Random Regression Tests
	auto benchmark_random_small_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::random_cache>(p_state, small_regression);
	}

	auto benchmark_random_medium_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::random_cache>(p_state, medium_regression);
	}

	auto benchmark_random_large_regression(benchmark::State& p_state) -> void
	{
		benchmark_baseline_performance<cache_engine::algorithm::random_cache>(p_state, large_regression);
	}

	auto benchmark_random_sequential_regression(benchmark::State& p_state) -> void
	{
		benchmark_sequential_regression<cache_engine::algorithm::random_cache>(p_state);
	}

	auto benchmark_random_hotcold_regression(benchmark::State& p_state) -> void
	{
		benchmark_hotcold_regression<cache_engine::algorithm::random_cache>(p_state);
	}

	auto benchmark_random_capacity_boundary(benchmark::State& p_state) -> void
	{
		benchmark_capacity_boundary<cache_engine::algorithm::random_cache>(p_state);
	}

}	// namespace cache_regression

// Register LRU regression benchmarks
BENCHMARK(cache_regression::benchmark_lru_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lru_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lru_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lru_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lru_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lru_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register FIFO regression benchmarks
BENCHMARK(cache_regression::benchmark_fifo_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_fifo_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_fifo_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_fifo_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_fifo_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_fifo_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register LFU regression benchmarks
BENCHMARK(cache_regression::benchmark_lfu_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lfu_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lfu_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lfu_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lfu_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_lfu_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MFU regression benchmarks
BENCHMARK(cache_regression::benchmark_mfu_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mfu_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mfu_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mfu_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mfu_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mfu_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MRU regression benchmarks
BENCHMARK(cache_regression::benchmark_mru_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mru_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mru_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mru_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mru_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_mru_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register Random regression benchmarks
BENCHMARK(cache_regression::benchmark_random_small_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_random_medium_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_random_large_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_random_sequential_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_random_hotcold_regression)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_regression::benchmark_random_capacity_boundary)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();