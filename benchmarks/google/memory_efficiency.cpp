/**
 * @file memory_efficiency.cpp
 * @brief Memory usage analysis and efficiency benchmarks for all cache algorithms
 * 
 * This file implements benchmarks that measure memory usage patterns, allocation
 * overhead, and memory efficiency characteristics of different cache algorithms.
 */

#include <benchmark/benchmark.h>
#include <cache_engine/cache.hpp>
#include <cache_engine/policies/all_policies.hpp>
#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <chrono>

namespace cache_memory
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
	 * @brief Memory profiling configuration
	 */
	struct memory_config
	{
		std::size_t cache_size;
		std::size_t key_range;
		std::size_t value_size;		// Size in bytes of each value
		std::size_t iterations;
		std::string test_name;
	};

	/**
	 * @brief Simple memory tracker for basic memory usage estimation
	 */
	class memory_tracker
	{
	private:
		std::size_t m_peak_memory;
		std::size_t m_current_memory;
		std::size_t m_allocations;
		std::size_t m_deallocations;

	public:
		memory_tracker() : m_peak_memory(0), m_current_memory(0), m_allocations(0), m_deallocations(0) {}

		auto record_allocation(std::size_t p_size) -> void
		{
			m_current_memory += p_size;
			m_allocations++;
			if (m_current_memory > m_peak_memory)
			{
				m_peak_memory = m_current_memory;
			}
		}

		auto record_deallocation(std::size_t p_size) -> void
		{
			if (m_current_memory >= p_size)
			{
				m_current_memory -= p_size;
			}
			m_deallocations++;
		}

		auto get_peak_memory() const -> std::size_t { return m_peak_memory; }
		auto get_current_memory() const -> std::size_t { return m_current_memory; }
		auto get_allocations() const -> std::size_t { return m_allocations; }
		auto get_deallocations() const -> std::size_t { return m_deallocations; }

		auto reset() -> void
		{
			m_peak_memory = 0;
			m_current_memory = 0;
			m_allocations = 0;
			m_deallocations = 0;
		}
	};

	/**
	 * @brief Value type with configurable memory footprint
	 */
	class variable_size_value
	{
	private:
		std::vector<char> m_data;

	public:
		variable_size_value() : m_data(64, 'A') {}  // Default constructor with reasonable size
		explicit variable_size_value(std::size_t p_size) : m_data(p_size, 'A') {}

		variable_size_value(const variable_size_value& p_other) = default;
		auto operator=(const variable_size_value& p_other) -> variable_size_value& = default;

		variable_size_value(variable_size_value&& p_other) noexcept = default;
		auto operator=(variable_size_value&& p_other) noexcept -> variable_size_value& = default;

		~variable_size_value() = default;

		auto size() const -> std::size_t { return m_data.size(); }
		auto data() const -> const char* { return m_data.data(); }
	};

	/**
	 * @brief Estimate memory overhead per cache entry
	 */
	template<typename algorithm_t>
	auto estimate_memory_overhead(const memory_config& p_config) -> std::size_t
	{
		using cache_t = cache_engine::cache<std::int32_t, variable_size_value, algorithm_t>;
		
		// Create cache and fill it completely
		cache_t cache(p_config.cache_size);
		
		for (std::size_t idx_for = 0; idx_for < p_config.cache_size; ++idx_for)
		{
			const auto key = static_cast<std::int32_t>(idx_for);
			cache.put(key, variable_size_value(p_config.value_size));
		}

		// Estimate total memory usage
		// This is a rough estimate - actual memory usage tracking would require
		// custom allocators or external memory profiling tools
		const std::size_t key_size = sizeof(std::int32_t);
		const std::size_t value_size = p_config.value_size + sizeof(variable_size_value);
		const std::size_t data_memory = p_config.cache_size * (key_size + value_size);
		
		// Different algorithms have different overhead structures
		std::size_t estimated_overhead = 0;
		
		// These are rough estimates based on the data structures used
		if (std::is_same<algorithm_t, cache_engine::algorithm::lru>::value ||
			std::is_same<algorithm_t, cache_engine::algorithm::mru>::value)
		{
			// LRU/MRU use list + unordered_map with iterators
			estimated_overhead = p_config.cache_size * (sizeof(std::list<std::int32_t>::iterator) + 
														sizeof(std::pair<variable_size_value, typename std::list<std::int32_t>::iterator>) +
														32); // Hash table overhead estimate
		}
		else if (std::is_same<algorithm_t, cache_engine::algorithm::fifo>::value)
		{
			// FIFO uses queue + unordered_map
			estimated_overhead = p_config.cache_size * (32 + // Hash table overhead
														16); // Queue overhead
		}
		else if (std::is_same<algorithm_t, cache_engine::algorithm::lfu>::value ||
				 std::is_same<algorithm_t, cache_engine::algorithm::mfu>::value)
		{
			// LFU/MFU use frequency map + unordered_map with frequency tracking
			estimated_overhead = p_config.cache_size * (sizeof(std::size_t) + // Frequency counter
														32 + // Hash table overhead
														24); // Map overhead for frequency tracking
		}
		else if (std::is_same<algorithm_t, cache_engine::algorithm::random_cache>::value)
		{
			// Random uses vector + unordered_map
			estimated_overhead = p_config.cache_size * (sizeof(std::int32_t) + // Vector entry
														32); // Hash table overhead
		}

		return data_memory + estimated_overhead;
	}

	/**
	 * @brief Memory usage benchmark template
	 */
	template<typename algorithm_t>
	auto benchmark_memory_usage(benchmark::State& p_state, const memory_config& p_config) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, variable_size_value, algorithm_t>;
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(p_config.key_range - 1));

		// Track allocation patterns
		std::size_t total_operations = 0;
		std::size_t memory_allocations = 0;

		for (auto _ : p_state)
		{
			// Create cache for this iteration
			cache_t cache(p_config.cache_size);
			
			// Fill cache with random data
			for (std::size_t idx_for = 0; idx_for < p_config.iterations; ++idx_for)
			{
				const auto key = key_dist(rng);
				cache.put(key, variable_size_value(p_config.value_size));
				++memory_allocations;
			}

			// Perform get operations to trigger memory access patterns
			for (std::size_t idx_for = 0; idx_for < p_config.iterations / 2; ++idx_for)
			{
				const auto key = key_dist(rng);
				try
				{
					benchmark::DoNotOptimize(cache.get(key));
				}
				catch (const std::out_of_range&)
				{
					// Cache miss - expected
				}
			}

			total_operations += p_config.iterations + (p_config.iterations / 2);
		}

		// Estimate memory usage
		const std::size_t estimated_memory = estimate_memory_overhead<algorithm_t>(p_config);

		// Report memory metrics
		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["EstimatedMemoryKB"] = benchmark::Counter(static_cast<double>(estimated_memory) / 1024.0, benchmark::Counter::kAvgThreads);
		p_state.counters["MemoryPerEntry"] = benchmark::Counter(static_cast<double>(estimated_memory) / static_cast<double>(p_config.cache_size), benchmark::Counter::kAvgThreads);
		p_state.counters["ValueSize"] = benchmark::Counter(static_cast<double>(p_config.value_size), benchmark::Counter::kAvgThreads);
		p_state.counters["CacheSize"] = benchmark::Counter(static_cast<double>(p_config.cache_size), benchmark::Counter::kAvgThreads);
	}

	/**
	 * @brief Memory allocation pattern benchmark
	 */
	template<typename algorithm_t>
	auto benchmark_allocation_pattern(benchmark::State& p_state, std::size_t p_cache_size, std::size_t p_operations) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		std::mt19937 rng(42);
		std::uniform_int_distribution<std::int32_t> key_dist(0, static_cast<std::int32_t>(p_cache_size * 10));

		std::size_t total_operations = 0;
		std::size_t allocation_events = 0;

		for (auto _ : p_state)
		{
			cache_t cache(p_cache_size);
			
			// Simulate allocation-heavy workload
			for (std::size_t idx_for = 0; idx_for < p_operations; ++idx_for)
			{
				const auto key = key_dist(rng);
				const std::string value = "allocation_test_value_" + std::to_string(key);
				cache.put(key, value);
				++allocation_events;
			}

			total_operations += p_operations;
		}

		p_state.SetItemsProcessed(static_cast<std::int64_t>(total_operations));
		p_state.counters["AllocationEvents"] = benchmark::Counter(static_cast<double>(allocation_events), benchmark::Counter::kAvgThreads);
		p_state.counters["AllocationsPerOp"] = benchmark::Counter(static_cast<double>(allocation_events) / static_cast<double>(total_operations), benchmark::Counter::kAvgThreads);
	}

	// Memory configuration scenarios
	const memory_config small_values = {1000, 5000, 64, 10000, "SmallValues"};
	const memory_config medium_values = {1000, 5000, 1024, 10000, "MediumValues"};
	const memory_config large_values = {1000, 5000, 8192, 10000, "LargeValues"};
	const memory_config small_cache = {100, 500, 256, 5000, "SmallCache"};
	const memory_config large_cache = {10000, 50000, 256, 50000, "LargeCache"};

	// LRU Memory Benchmarks
	auto benchmark_lru_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lru>(p_state, small_values);
	}

	auto benchmark_lru_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lru>(p_state, medium_values);
	}

	auto benchmark_lru_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lru>(p_state, large_values);
	}

	auto benchmark_lru_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lru>(p_state, small_cache);
	}

	auto benchmark_lru_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lru>(p_state, large_cache);
	}

	auto benchmark_lru_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::lru>(p_state, 1000, 10000);
	}

	// FIFO Memory Benchmarks
	auto benchmark_fifo_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::fifo>(p_state, small_values);
	}

	auto benchmark_fifo_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::fifo>(p_state, medium_values);
	}

	auto benchmark_fifo_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::fifo>(p_state, large_values);
	}

	auto benchmark_fifo_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::fifo>(p_state, small_cache);
	}

	auto benchmark_fifo_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::fifo>(p_state, large_cache);
	}

	auto benchmark_fifo_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::fifo>(p_state, 1000, 10000);
	}

	// LFU Memory Benchmarks
	auto benchmark_lfu_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lfu>(p_state, small_values);
	}

	auto benchmark_lfu_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lfu>(p_state, medium_values);
	}

	auto benchmark_lfu_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lfu>(p_state, large_values);
	}

	auto benchmark_lfu_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lfu>(p_state, small_cache);
	}

	auto benchmark_lfu_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::lfu>(p_state, large_cache);
	}

	auto benchmark_lfu_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::lfu>(p_state, 1000, 10000);
	}

	// MFU Memory Benchmarks
	auto benchmark_mfu_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mfu>(p_state, small_values);
	}

	auto benchmark_mfu_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mfu>(p_state, medium_values);
	}

	auto benchmark_mfu_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mfu>(p_state, large_values);
	}

	auto benchmark_mfu_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mfu>(p_state, small_cache);
	}

	auto benchmark_mfu_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mfu>(p_state, large_cache);
	}

	auto benchmark_mfu_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::mfu>(p_state, 1000, 10000);
	}

	// MRU Memory Benchmarks
	auto benchmark_mru_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mru>(p_state, small_values);
	}

	auto benchmark_mru_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mru>(p_state, medium_values);
	}

	auto benchmark_mru_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mru>(p_state, large_values);
	}

	auto benchmark_mru_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mru>(p_state, small_cache);
	}

	auto benchmark_mru_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::mru>(p_state, large_cache);
	}

	auto benchmark_mru_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::mru>(p_state, 1000, 10000);
	}

	// Random Memory Benchmarks
	auto benchmark_random_memory_small_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::random_cache>(p_state, small_values);
	}

	auto benchmark_random_memory_medium_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::random_cache>(p_state, medium_values);
	}

	auto benchmark_random_memory_large_values(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::random_cache>(p_state, large_values);
	}

	auto benchmark_random_memory_small_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::random_cache>(p_state, small_cache);
	}

	auto benchmark_random_memory_large_cache(benchmark::State& p_state) -> void
	{
		benchmark_memory_usage<cache_engine::algorithm::random_cache>(p_state, large_cache);
	}

	auto benchmark_random_allocation_pattern(benchmark::State& p_state) -> void
	{
		benchmark_allocation_pattern<cache_engine::algorithm::random_cache>(p_state, 1000, 10000);
	}

}	// namespace cache_memory

// Register LRU memory benchmarks
BENCHMARK(cache_memory::benchmark_lru_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lru_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lru_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lru_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lru_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lru_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register FIFO memory benchmarks
BENCHMARK(cache_memory::benchmark_fifo_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_fifo_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_fifo_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_fifo_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_fifo_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_fifo_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register LFU memory benchmarks
BENCHMARK(cache_memory::benchmark_lfu_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lfu_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lfu_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lfu_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lfu_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_lfu_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MFU memory benchmarks
BENCHMARK(cache_memory::benchmark_mfu_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mfu_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mfu_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mfu_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mfu_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mfu_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MRU memory benchmarks
BENCHMARK(cache_memory::benchmark_mru_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mru_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mru_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mru_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mru_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_mru_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register Random memory benchmarks
BENCHMARK(cache_memory::benchmark_random_memory_small_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_random_memory_medium_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_random_memory_large_values)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_random_memory_small_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_random_memory_large_cache)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_memory::benchmark_random_allocation_pattern)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();