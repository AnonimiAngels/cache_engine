/**
 * @file scaling_analysis.cpp
 * @brief Performance scaling analysis for cache algorithms across different cache sizes
 * 
 * This file implements benchmarks that analyze how cache performance scales with
 * cache size, key range, and operation complexity to identify performance characteristics
 * and bottlenecks for each algorithm.
 */

#include <benchmark/benchmark.h>
#include <cache_engine/cache.hpp>
#include <cache_engine/policies/all_policies.hpp>
#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <unordered_set>

namespace cache_scaling
{
	/**
	 * @brief Scaling test configuration
	 */
	struct scaling_config
	{
		std::vector<std::size_t> cache_sizes;
		std::size_t operations_per_size;
		double key_range_multiplier;  // Key range = cache_size * multiplier
		std::string test_name;
	};

	/**
	 * @brief Performance metrics collection
	 */
	struct performance_metrics
	{
		double throughput_ops_per_sec;
		double latency_avg_ns;
		double latency_p95_ns;
		double memory_usage_mb;
		std::size_t cache_size;
	};

	/**
	 * @brief Scaling performance benchmark template
	 */
	template<typename algorithm_t>
	auto benchmark_scaling_performance(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = static_cast<std::size_t>(p_state.range(0));
		const std::size_t key_range = static_cast<std::size_t>(static_cast<double>(cache_size) * 5.0);
		const std::size_t operations = cache_size * 100;  // Scale operations with cache size
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(key_range - 1));
		std::uniform_real_distribution<double> op_dist(0.0, 1.0);
		
		// Generate test data
		std::vector<std::pair<bool, std::int32_t>> test_operations;
		test_operations.reserve(operations);
		
		for (std::size_t idx_for = 0; idx_for < operations; ++idx_for)
		{
			const bool is_get = op_dist(rng) < 0.7;  // 70% get, 30% put
			const auto key = key_dist(rng);
			test_operations.emplace_back(is_get, key);
		}

		std::size_t total_operations = 0;
		std::size_t hit_count = 0;
		std::size_t miss_count = 0;

		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			// Pre-populate cache with some data
			for (std::size_t idx_for = 0; idx_for < cache_size / 2; ++idx_for)
			{
				const auto key = static_cast<std::int32_t>(idx_for);
				cache.put(key, "initial_value_" + std::to_string(key));
			}

			// Execute test operations
			for (const auto& operation : test_operations)
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
					cache.put(key, "test_value_" + std::to_string(key));
				}
			}
			
			total_operations += operations;
		}

		// Report scaling metrics
		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.SetBytesProcessed(static_cast<std::int64_t>(total_operations * (sizeof(std::int32_t) + 20)));
		
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["KeyRange"] = benchmark::Counter(static_cast<double>(key_range), benchmark::Counter::kAvgThreads);
		p_state.counters["OpsPerSize"] = benchmark::Counter(static_cast<double>(operations) / static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
		
		if (hit_count + miss_count > 0)
		{
			const double hit_rate = static_cast<double>(hit_count) / static_cast<double>(hit_count + miss_count);
			p_state.counters["HitRate"] = benchmark::Counter(hit_rate, benchmark::Counter::kAvgThreads);
		}
	}

	/**
	 * @brief Cache capacity stress test
	 */
	template<typename algorithm_t>
	auto benchmark_capacity_stress(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = static_cast<std::size_t>(p_state.range(0));
		const std::size_t stress_operations = cache_size * 10;  // 10x capacity in operations
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(cache_size * 20));

		std::size_t total_operations = 0;
		std::size_t evictions = 0;

		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			// Stress test with many more operations than cache capacity
			for (std::size_t idx_for = 0; idx_for < stress_operations; ++idx_for)
			{
				const auto key = key_dist(rng);
				cache.put(key, "stress_value_" + std::to_string(key));
				
				// Estimate evictions (rough approximation)
				if (idx_for > cache_size)
				{
					++evictions;
				}
			}
			
			total_operations += stress_operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["StressRatio"] = benchmark::Counter(static_cast<double>(stress_operations) / static_cast<double>(cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["EstimatedEvictions"] = benchmark::Counter(static_cast<double>(evictions), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Key range impact analysis
	 */
	template<typename algorithm_t>
	auto benchmark_key_range_impact(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = 1000;  // Fixed cache size
		const std::size_t key_range = static_cast<std::size_t>(p_state.range(0));
		const std::size_t operations = 10000;
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(key_range - 1));

		std::size_t total_operations = 0;
		std::size_t unique_keys_accessed = 0;

		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			std::unordered_set<std::int32_t> accessed_keys;
			
			for (std::size_t idx_for = 0; idx_for < operations; ++idx_for)
			{
				const auto key = key_dist(rng);
				accessed_keys.insert(key);
				
				if ((idx_for % 10) < 7)  // 70% get operations
				{
					try
					{
						benchmark::DoNotOptimize(cache.get(key));
					}
					catch (const std::out_of_range&)
					{
						// Cache miss
					}
				}
				else  // 30% put operations
				{
					cache.put(key, "range_value_" + std::to_string(key));
				}
			}
			
			unique_keys_accessed = accessed_keys.size();
			total_operations += operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["KeyRange"] = benchmark::Counter(static_cast<double>(key_range), benchmark::Counter::kAvgThreads);
		p_state.counters["UniqueKeys"] = benchmark::Counter(static_cast<double>(unique_keys_accessed), benchmark::Counter::kAvgThreads);
		p_state.counters["KeyDiversity"] = benchmark::Counter(static_cast<double>(unique_keys_accessed) / static_cast<double>(key_range), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Workload intensity scaling
	 */
	template<typename algorithm_t>
	auto benchmark_workload_intensity(benchmark::State& p_state) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		const std::size_t cache_size = 1000;
		const std::size_t base_operations = 1000;
		const std::size_t intensity_multiplier = static_cast<std::size_t>(p_state.range(0));
		const std::size_t total_operations = base_operations * intensity_multiplier;
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(cache_size * 5));

		std::size_t operations_completed = 0;

		for (auto _ : p_state)
		{
			cache_t cache(cache_size);
			
			for (std::size_t idx_for = 0; idx_for < total_operations; ++idx_for)
			{
				const auto key = key_dist(rng);
				
				if ((idx_for % 10) < 6)  // 60% get operations
				{
					try
					{
						benchmark::DoNotOptimize(cache.get(key));
					}
					catch (const std::out_of_range&)
					{
						// Cache miss
					}
				}
				else  // 40% put operations
				{
					cache.put(key, "intensity_value_" + std::to_string(key));
				}
			}
			
			operations_completed += total_operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(operations_completed));
		p_state.counters["IntensityMultiplier"] = benchmark::Counter(static_cast<double>(intensity_multiplier), benchmark::Counter::kAvgThreads);
		p_state.counters["TotalOperations"] = benchmark::Counter(static_cast<double>(total_operations), benchmark::Counter::kAvgThreads);
	}

	// Forward declarations for scaling benchmark functions
	auto benchmark_lru_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_lru_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_lru_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_lru_workload_intensity(benchmark::State& p_state) -> void;
	auto benchmark_fifo_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_fifo_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_fifo_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_fifo_workload_intensity(benchmark::State& p_state) -> void;
	auto benchmark_lfu_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_lfu_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_lfu_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_lfu_workload_intensity(benchmark::State& p_state) -> void;
	auto benchmark_mfu_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_mfu_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_mfu_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_mfu_workload_intensity(benchmark::State& p_state) -> void;
	auto benchmark_mru_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_mru_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_mru_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_mru_workload_intensity(benchmark::State& p_state) -> void;
	auto benchmark_random_scaling_performance(benchmark::State& p_state) -> void;
	auto benchmark_random_capacity_stress(benchmark::State& p_state) -> void;
	auto benchmark_random_key_range_impact(benchmark::State& p_state) -> void;
	auto benchmark_random_workload_intensity(benchmark::State& p_state) -> void;

	// Global template benchmark functions for LRU
	auto benchmark_lru_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::lru>(p_state);
	}

	auto benchmark_lru_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::lru>(p_state);
	}

	auto benchmark_lru_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::lru>(p_state);
	}

	auto benchmark_lru_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::lru>(p_state);
	}

	// Global template benchmark functions for FIFO
	auto benchmark_fifo_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::fifo>(p_state);
	}

	auto benchmark_fifo_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::fifo>(p_state);
	}

	auto benchmark_fifo_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::fifo>(p_state);
	}

	auto benchmark_fifo_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::fifo>(p_state);
	}

	// Global template benchmark functions for LFU
	auto benchmark_lfu_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::lfu>(p_state);
	}

	auto benchmark_lfu_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::lfu>(p_state);
	}

	auto benchmark_lfu_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::lfu>(p_state);
	}

	auto benchmark_lfu_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::lfu>(p_state);
	}

	// Global template benchmark functions for MFU
	auto benchmark_mfu_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::mfu>(p_state);
	}

	auto benchmark_mfu_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::mfu>(p_state);
	}

	auto benchmark_mfu_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::mfu>(p_state);
	}

	auto benchmark_mfu_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::mfu>(p_state);
	}

	// Global template benchmark functions for MRU
	auto benchmark_mru_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::mru>(p_state);
	}

	auto benchmark_mru_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::mru>(p_state);
	}

	auto benchmark_mru_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::mru>(p_state);
	}

	auto benchmark_mru_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::mru>(p_state);
	}

	// Global template benchmark functions for Random
	auto benchmark_random_scaling_performance(benchmark::State& p_state) -> void
	{
		benchmark_scaling_performance<cache_engine::algorithm::random_cache>(p_state);
	}

	auto benchmark_random_capacity_stress(benchmark::State& p_state) -> void
	{
		benchmark_capacity_stress<cache_engine::algorithm::random_cache>(p_state);
	}

	auto benchmark_random_key_range_impact(benchmark::State& p_state) -> void
	{
		benchmark_key_range_impact<cache_engine::algorithm::random_cache>(p_state);
	}

	auto benchmark_random_workload_intensity(benchmark::State& p_state) -> void
	{
		benchmark_workload_intensity<cache_engine::algorithm::random_cache>(p_state);
	}

}	// namespace cache_scaling

// LRU Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_lru_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lru_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lru_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lru_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

// FIFO Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_fifo_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_fifo_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_fifo_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_fifo_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

// LFU Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_lfu_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lfu_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lfu_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_lfu_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

// MFU Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_mfu_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mfu_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mfu_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mfu_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

// MRU Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_mru_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mru_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mru_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_mru_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Random Scaling Benchmarks
BENCHMARK(cache_scaling::benchmark_random_scaling_performance)
	->RangeMultiplier(10)->Range(10, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_random_capacity_stress)
	->RangeMultiplier(10)->Range(100, 10000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_random_key_range_impact)
	->RangeMultiplier(10)->Range(1000, 100000)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(cache_scaling::benchmark_random_workload_intensity)
	->RangeMultiplier(2)->Range(1, 32)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();