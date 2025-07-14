/**
 * @file cache_performance.cpp
 * @brief Comprehensive throughput benchmarks for all cache algorithms
 * 
 * This file implements Google Benchmark tests that measure the throughput performance
 * of all cache engine algorithms under various scenarios including different cache sizes,
 * hit/miss ratios, and key distributions.
 */

#include <benchmark/benchmark.h>
#include <cache_engine/cache.hpp>
#include <random>
#include <vector>
#include <string>
#include <cstdint>

namespace cache_benchmark
{
	/**
	 * @brief Test scenario configuration
	 */
	struct benchmark_config
	{
		std::size_t cache_size;
		std::size_t key_range;
		std::size_t operations;
		double hit_ratio;
	};

	/**
	 * @brief Key distribution patterns for testing
	 */
	enum class key_distribution
	{
		uniform,	// Uniform random distribution
		normal,		// Normal (Gaussian) distribution
		zipfian		// Zipfian distribution (80/20 rule)
	};

	/**
	 * @brief Generate test keys based on distribution pattern
	 */
	class key_generator
	{
	public:
		using key_t = std::int32_t;

	private:
		mutable std::mt19937 m_rng;
		std::size_t m_key_range;
		key_distribution m_distribution;

	public:
		explicit key_generator(std::size_t p_key_range, key_distribution p_dist = key_distribution::uniform)
			: m_rng(42), m_key_range(p_key_range), m_distribution(p_dist)
		{
		}

		auto generate() const -> key_t
		{
			switch (m_distribution)
			{
				case key_distribution::uniform:
				{
					std::uniform_int_distribution<key_t> dist(0, static_cast<key_t>(m_key_range - 1));
					return dist(m_rng);
				}
				case key_distribution::normal:
				{
					std::normal_distribution<double> dist(static_cast<double>(m_key_range) / 2.0, static_cast<double>(m_key_range) / 6.0);
					const auto key = static_cast<key_t>(std::max(0.0, std::min(static_cast<double>(m_key_range - 1), dist(m_rng))));
					return key;
				}
				case key_distribution::zipfian:
				{
					// Simplified Zipfian: 80% of accesses to first 20% of keys
					std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
					if (prob_dist(m_rng) < 0.8)
					{
						std::uniform_int_distribution<key_t> hot_dist(0, static_cast<key_t>(m_key_range / 5));
						return hot_dist(m_rng);
					}
					else
					{
						std::uniform_int_distribution<key_t> cold_dist(static_cast<key_t>(m_key_range / 5), static_cast<key_t>(m_key_range - 1));
						return cold_dist(m_rng);
					}
				}
			}
			return 0;
		}

		auto generate_batch(std::size_t p_count) const -> std::vector<key_t>
		{
			std::vector<key_t> keys;
			keys.reserve(p_count);
			for (std::size_t idx_for = 0; idx_for < p_count; ++idx_for)
			{
				keys.push_back(generate());
			}
			return keys;
		}
	};

	/**
	 * @brief Benchmark fixture for LRU cache performance
	 */
	template<typename algorithm_t>
	auto benchmark_cache_throughput(benchmark::State& p_state, const benchmark_config& p_config, key_distribution p_dist) -> void
	{
		using cache_t = cache_engine::cache<std::int32_t, std::string, algorithm_t>;
		
		// Initialize cache
		cache_t cache(p_config.cache_size);
		
		// Generate test keys
		key_generator key_gen(p_config.key_range, p_dist);
		const auto test_keys = key_gen.generate_batch(p_config.operations);
		
		// Pre-populate cache to achieve desired hit ratio
		const std::size_t populate_count = static_cast<std::size_t>(static_cast<double>(p_config.cache_size) * p_config.hit_ratio);
		for (std::size_t idx_for = 0; idx_for < populate_count; ++idx_for)
		{
			const auto key = static_cast<std::int32_t>(idx_for % p_config.key_range);
			cache.put(key, "value_" + std::to_string(key));
		}

		// Benchmark loop
		std::size_t operation_count = 0;
		for (auto _ : p_state)
		{
			const auto key = test_keys[operation_count % test_keys.size()];
			
			// Mix of put and get operations (70% get, 30% put)
			if ((operation_count % 10) < 7)
			{
				try
				{
					benchmark::DoNotOptimize(cache.get(key));
				}
				catch (const std::out_of_range&)
				{
					// Cache miss - expected behavior
				}
			}
			else
			{
				cache.put(key, "value_" + std::to_string(key));
			}
			
			++operation_count;
		}

		// Report performance metrics
		p_state.SetItemsProcessed(static_cast<std::int64_t>(operation_count));
		p_state.SetBytesProcessed(static_cast<std::int64_t>(operation_count * (sizeof(std::int32_t) + 20))); // Approximate memory per operation
	}

	// Forward declarations for benchmark functions
	auto benchmark_lru_small(benchmark::State& p_state) -> void;
	auto benchmark_lru_medium(benchmark::State& p_state) -> void;
	auto benchmark_lru_large(benchmark::State& p_state) -> void;
	auto benchmark_lru_xlarge(benchmark::State& p_state) -> void;
	auto benchmark_lru_xxlarge(benchmark::State& p_state) -> void;
	auto benchmark_fifo_small(benchmark::State& p_state) -> void;
	auto benchmark_fifo_medium(benchmark::State& p_state) -> void;
	auto benchmark_fifo_large(benchmark::State& p_state) -> void;
	auto benchmark_fifo_xlarge(benchmark::State& p_state) -> void;
	auto benchmark_fifo_xxlarge(benchmark::State& p_state) -> void;
	auto benchmark_lfu_small(benchmark::State& p_state) -> void;
	auto benchmark_lfu_medium(benchmark::State& p_state) -> void;
	auto benchmark_lfu_large(benchmark::State& p_state) -> void;
	auto benchmark_lfu_xlarge(benchmark::State& p_state) -> void;
	auto benchmark_mfu_small(benchmark::State& p_state) -> void;
	auto benchmark_mfu_medium(benchmark::State& p_state) -> void;
	auto benchmark_mfu_large(benchmark::State& p_state) -> void;
	auto benchmark_mfu_xlarge(benchmark::State& p_state) -> void;
	auto benchmark_mru_small(benchmark::State& p_state) -> void;
	auto benchmark_mru_medium(benchmark::State& p_state) -> void;
	auto benchmark_mru_large(benchmark::State& p_state) -> void;
	auto benchmark_mru_xlarge(benchmark::State& p_state) -> void;
	auto benchmark_random_small(benchmark::State& p_state) -> void;
	auto benchmark_random_medium(benchmark::State& p_state) -> void;
	auto benchmark_random_large(benchmark::State& p_state) -> void;
	auto benchmark_random_xlarge(benchmark::State& p_state) -> void;

	// LRU Cache Benchmarks
	auto benchmark_lru_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lru>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lru_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lru>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lru_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lru>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lru_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lru>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lru_xxlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lru>(p_state, {100000, 500000, 10000000, 0.8}, key_distribution::uniform);
	}

	// FIFO Cache Benchmarks
	auto benchmark_fifo_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::fifo>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_fifo_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::fifo>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_fifo_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::fifo>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_fifo_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::fifo>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_fifo_xxlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::fifo>(p_state, {100000, 500000, 10000000, 0.8}, key_distribution::uniform);
	}

	// LFU Cache Benchmarks
	auto benchmark_lfu_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lfu>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lfu_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lfu>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lfu_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lfu>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_lfu_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::lfu>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

	// MFU Cache Benchmarks
	auto benchmark_mfu_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mfu>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mfu_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mfu>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mfu_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mfu>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mfu_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mfu>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

	// MRU Cache Benchmarks
	auto benchmark_mru_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mru>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mru_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mru>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mru_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mru>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_mru_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::mru>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

	// Random Cache Benchmarks
	auto benchmark_random_small(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::random_cache>(p_state, {10, 50, 1000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_random_medium(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::random_cache>(p_state, {100, 500, 10000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_random_large(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::random_cache>(p_state, {1000, 5000, 100000, 0.8}, key_distribution::uniform);
	}

	auto benchmark_random_xlarge(benchmark::State& p_state) -> void
	{
		benchmark_cache_throughput<cache_engine::algorithm::random_cache>(p_state, {10000, 50000, 1000000, 0.8}, key_distribution::uniform);
	}

}	// namespace cache_benchmark

// Register LRU benchmarks
BENCHMARK(cache_benchmark::benchmark_lru_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lru_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lru_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lru_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lru_xxlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register FIFO benchmarks  
BENCHMARK(cache_benchmark::benchmark_fifo_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_fifo_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_fifo_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_fifo_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_fifo_xxlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register LFU benchmarks
BENCHMARK(cache_benchmark::benchmark_lfu_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lfu_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lfu_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_lfu_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MFU benchmarks
BENCHMARK(cache_benchmark::benchmark_mfu_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mfu_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mfu_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mfu_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register MRU benchmarks
BENCHMARK(cache_benchmark::benchmark_mru_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mru_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mru_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_mru_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

// Register Random benchmarks
BENCHMARK(cache_benchmark::benchmark_random_small)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_random_medium)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_random_large)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK(cache_benchmark::benchmark_random_xlarge)->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();