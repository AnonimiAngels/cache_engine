#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "cache_engine/cache.hpp"

namespace benchmark
{
	// Constants for formatting and calculations
	constexpr std::size_t algorithm_width	= 10;
	constexpr std::size_t hit_rate_width	= 12;
	constexpr std::size_t throughput_width	= 15;
	constexpr std::size_t separator_width	= 52;
	constexpr double percentage_multiplier	= 100.0;
	constexpr double nanoseconds_per_second = 1e9;
	constexpr int precision_decimal			= 1;
	constexpr int precision_integer			= 0;

	struct cache_stats
	{
	private:
		std::size_t m_hits						  = 0;
		std::size_t m_misses					  = 0;
		std::chrono::nanoseconds m_total_put_time = std::chrono::nanoseconds::zero();
		std::chrono::nanoseconds m_total_get_time = std::chrono::nanoseconds::zero();
		std::size_t m_put_operations			  = 0;
		std::size_t m_get_operations			  = 0;

	public:
		auto increment_hits() -> void { m_hits++; }

		auto increment_misses() -> void { m_misses++; }

		auto increment_put_operations() -> void { m_put_operations++; }

		auto increment_get_operations() -> void { m_get_operations++; }

		auto add_put_time(std::chrono::nanoseconds p_time) -> void { m_total_put_time += p_time; }

		auto add_get_time(std::chrono::nanoseconds p_time) -> void { m_total_get_time += p_time; }

		auto get_hits() const -> std::size_t { return m_hits; }

		auto get_misses() const -> std::size_t { return m_misses; }

		auto hit_rate() const -> double
		{
			if (m_hits + m_misses == 0)
			{
				return 0.0;
			}
			return static_cast<double>(m_hits) / static_cast<double>(m_hits + m_misses);
		}

		auto avg_put_time_ns() const -> double
		{
			if (m_put_operations == 0)
			{
				return 0.0;
			}
			return static_cast<double>(m_total_put_time.count()) / static_cast<double>(m_put_operations);
		}

		auto avg_get_time_ns() const -> double
		{
			if (m_get_operations == 0)
			{
				return 0.0;
			}
			return static_cast<double>(m_total_get_time.count()) / static_cast<double>(m_get_operations);
		}

		auto put_throughput_ops_per_sec() const -> double
		{
			if (m_total_put_time.count() == 0)
			{
				return 0.0;
			}
			return static_cast<double>(m_put_operations) * nanoseconds_per_second / static_cast<double>(m_total_put_time.count());
		}

		auto get_throughput_ops_per_sec() const -> double
		{
			if (m_total_get_time.count() == 0)
			{
				return 0.0;
			}
			return static_cast<double>(m_get_operations) * nanoseconds_per_second / static_cast<double>(m_total_get_time.count());
		}
	};

	struct data_size_t
	{
	private:
		std::size_t m_value;

	public:
		explicit data_size_t(std::size_t p_value) : m_value(p_value) {}

		operator std::size_t() const { return m_value; }

		auto operator++() -> data_size_t&
		{
			++m_value;
			return *this;
		}

		auto operator++(int) -> data_size_t
		{
			data_size_t temp(*this);
			++m_value;
			return temp;
		}
	};

	struct key_range_t
	{
	private:
		std::size_t m_value;

	public:
		explicit key_range_t(std::size_t p_value) : m_value(p_value) {}

		operator std::size_t() const { return m_value; }
	};

	namespace
	{
		template <typename cache_t>
		auto benchmark_cache(cache_t& p_cache, const std::vector<std::pair<int, std::string>>& p_operations, const std::vector<int>& p_get_keys, const std::string& p_algorithm_name)
			-> cache_stats
		{
			cache_stats stats;

			std::cout << "Benchmarking " << p_algorithm_name << " cache..." << '\n';
			for (const auto& operation : p_operations)
			{
				const auto put_start = std::chrono::high_resolution_clock::now();
				p_cache.put(operation.first, operation.second);
				const auto put_end = std::chrono::high_resolution_clock::now();

				stats.add_put_time(std::chrono::duration_cast<std::chrono::nanoseconds>(put_end - put_start));
				stats.increment_put_operations();
			}

			for (const auto& key : p_get_keys)
			{
				const auto get_start = std::chrono::high_resolution_clock::now();
				try
				{
					p_cache.get(key);
					stats.increment_hits();
				}
				catch (const std::out_of_range&)
				{
					stats.increment_misses();
				}
				const auto get_end = std::chrono::high_resolution_clock::now();

				stats.add_get_time(std::chrono::duration_cast<std::chrono::nanoseconds>(get_end - get_start));
				stats.increment_get_operations();
			}

			return stats;
		}

		auto generate_test_data(data_size_t p_size, key_range_t p_key_range) -> std::vector<std::pair<int, std::string>>
		{
			std::vector<std::pair<int, std::string>> data;
			std::random_device random_device;
			std::mt19937 gen(random_device());
			std::uniform_int_distribution<int> dis(1, static_cast<int>(p_key_range));

			data.reserve(p_size);
			for (std::size_t idx_for = 0; idx_for < p_size; ++idx_for)
			{
				const int key			= dis(gen);
				const std::string value = "value_" + std::to_string(key);
				data.emplace_back(key, value);
			}

			return data;
		}

		auto generate_get_keys(data_size_t p_size, key_range_t p_key_range) -> std::vector<int>
		{
			std::vector<int> keys;
			std::random_device random_device;
			std::mt19937 gen(random_device());
			std::uniform_int_distribution<int> dis(1, static_cast<int>(p_key_range));

			keys.reserve(p_size);
			for (std::size_t idx_for = 0; idx_for < p_size; ++idx_for)
			{
				keys.push_back(dis(gen));
			}

			return keys;
		}

		auto print_results(const std::string& p_algorithm, const cache_stats& p_stats) -> void
		{
			std::cout << std::fixed << std::setprecision(2);
			std::cout << "=== " << p_algorithm << " Results ===" << '\n';
			std::cout << "Hit Rate: " << (p_stats.hit_rate() * percentage_multiplier) << "%" << '\n';
			std::cout << "Cache Hits: " << p_stats.get_hits() << ", Misses: " << p_stats.get_misses() << '\n';
			std::cout << "Avg PUT time: " << p_stats.avg_put_time_ns() << " ns" << '\n';
			std::cout << "Avg GET time: " << p_stats.avg_get_time_ns() << " ns" << '\n';
			std::cout << "PUT throughput: " << p_stats.put_throughput_ops_per_sec() << " ops/sec" << '\n';
			std::cout << "GET throughput: " << p_stats.get_throughput_ops_per_sec() << " ops/sec" << '\n';
			std::cout << '\n';
		}

		auto run_comprehensive_benchmark() -> void
		{
			constexpr std::size_t cache_size		 = 100;
			constexpr std::size_t num_operations	 = 10000000;
			constexpr std::size_t key_range			 = 500;
			constexpr std::size_t num_get_operations = 100000000;

			std::cout << "=== Comprehensive Cache Benchmark ===" << '\n';
			std::cout << "Cache Size: " << cache_size << '\n';
			std::cout << "PUT Operations: " << num_operations << '\n';
			std::cout << "GET Operations: " << num_get_operations << '\n';
			std::cout << "Key Range: 1-" << key_range << '\n';
			std::cout << '\n';

			const auto put_data = generate_test_data(data_size_t(num_operations), key_range_t(key_range));
			const auto get_keys = generate_get_keys(data_size_t(num_get_operations), key_range_t(key_range));

			cache_engine::cache<int, std::string, cache_engine::algorithm::lru> lru_cache(cache_size);
			const auto lru_stats = benchmark_cache(lru_cache, put_data, get_keys, "LRU");
			print_results("LRU", lru_stats);

			cache_engine::cache<int, std::string, cache_engine::algorithm::fifo> fifo_cache(cache_size);
			const auto fifo_stats = benchmark_cache(fifo_cache, put_data, get_keys, "FIFO");
			print_results("FIFO", fifo_stats);

			cache_engine::cache<int, std::string, cache_engine::algorithm::lfu> lfu_cache(cache_size);
			const auto lfu_stats = benchmark_cache(lfu_cache, put_data, get_keys, "LFU");
			print_results("LFU", lfu_stats);

			cache_engine::cache<int, std::string, cache_engine::algorithm::mfu> mfu_cache(cache_size);
			const auto mfu_stats = benchmark_cache(mfu_cache, put_data, get_keys, "MFU");
			print_results("MFU", mfu_stats);

			cache_engine::cache<int, std::string, cache_engine::algorithm::mru> mru_cache(cache_size);
			const auto mru_stats = benchmark_cache(mru_cache, put_data, get_keys, "MRU");
			print_results("MRU", mru_stats);

			cache_engine::cache<int, std::string, cache_engine::algorithm::random_cache> random_cache(cache_size);
			const auto random_stats = benchmark_cache(random_cache, put_data, get_keys, "RANDOM");
			print_results("RANDOM", random_stats);

			std::cout << "=== Performance Comparison ===" << '\n';
			std::cout << std::left << std::setw(algorithm_width) << "Algorithm" << std::setw(hit_rate_width) << "Hit Rate %" << std::setw(throughput_width) << "PUT ops/sec"
					  << std::setw(throughput_width) << "GET ops/sec" << '\n';
			std::cout << std::string(separator_width, '-') << '\n';

			const std::vector<std::pair<std::string, cache_stats>> all_stats = {
				{"LRU", lru_stats}, {"FIFO", fifo_stats}, {"LFU", lfu_stats}, {"MFU", mfu_stats}, {"MRU", mru_stats}, {"RANDOM", random_stats}};

			for (const auto& all_stat : all_stats)
			{
				const auto& algorithm = all_stat.first;
				const auto& stats	  = all_stat.second;
				std::cout << std::left << std::setw(algorithm_width) << algorithm << std::setw(hit_rate_width) << std::fixed << std::setprecision(precision_decimal)
						  << (stats.hit_rate() * percentage_multiplier) << std::setw(throughput_width) << std::fixed << std::setprecision(precision_integer)
						  << stats.put_throughput_ops_per_sec() << std::setw(throughput_width) << std::fixed << std::setprecision(precision_integer) << stats.get_throughput_ops_per_sec()
						  << '\n';
			}
		}

		auto test_algorithm_correctness() -> void
		{
			std::cout << "=== Algorithm Correctness Tests ===" << '\n';

			std::cout << "Testing FIFO eviction..." << '\n';
			cache_engine::cache<int, std::string, cache_engine::algorithm::fifo> fifo_test(2);
			fifo_test.put(1, "one");
			fifo_test.put(2, "two");
			fifo_test.put(3, "three");
			try
			{
				fifo_test.get(1);
				std::cout << "ERROR: Key 1 should have been evicted!" << '\n';
			}
			catch (const std::out_of_range&)
			{
				std::cout << "PASS: FIFO correctly evicted key 1" << '\n';
			}

			std::cout << "Testing LRU eviction..." << '\n';
			cache_engine::cache<int, std::string, cache_engine::algorithm::lru> lru_test(2);
			lru_test.put(1, "one");
			lru_test.put(2, "two");
			lru_test.get(1);
			lru_test.put(3, "three");
			try
			{
				lru_test.get(2);
				std::cout << "ERROR: Key 2 should have been evicted!" << '\n';
			}
			catch (const std::out_of_range&)
			{
				std::cout << "PASS: LRU correctly evicted key 2" << '\n';
			}

			std::cout << "Testing MRU eviction..." << '\n';
			cache_engine::cache<int, std::string, cache_engine::algorithm::mru> mru_test(2);
			mru_test.put(1, "one");
			mru_test.put(2, "two");
			mru_test.get(1);
			mru_test.put(3, "three");
			try
			{
				mru_test.get(1);
				std::cout << "ERROR: Key 1 should have been evicted!" << '\n';
			}
			catch (const std::out_of_range&)
			{
				std::cout << "PASS: MRU correctly evicted key 1" << '\n';
			}

			std::cout << "Testing LFU frequency tracking..." << '\n';
			cache_engine::cache<int, std::string, cache_engine::algorithm::lfu> lfu_test(2);
			lfu_test.put(1, "one");
			lfu_test.put(2, "two");
			lfu_test.get(1);
			lfu_test.get(1);
			lfu_test.put(3, "three");
			try
			{
				lfu_test.get(2);
				std::cout << "ERROR: Key 2 should have been evicted!" << '\n';
			}
			catch (const std::out_of_range&)
			{
				std::cout << "PASS: LFU correctly evicted least frequent key 2" << '\n';
			}

			std::cout << "Testing key update behavior..." << '\n';
			cache_engine::cache<int, std::string, cache_engine::algorithm::fifo> update_test(2);
			update_test.put(1, "one");
			update_test.put(2, "two");
			update_test.put(1, "one_updated");
			try
			{
				const std::string result = update_test.get(1);
				if (result == "one_updated")
				{
					std::cout << "PASS: Key update working correctly" << '\n';
				}
				else
				{
					std::cout << "ERROR: Key update failed" << '\n';
				}
			}
			catch (const std::out_of_range&)
			{
				std::cout << "ERROR: Updated key should still be accessible" << '\n';
			}

			std::cout << '\n';
		}
	}	 // anonymous namespace
}	 // namespace benchmark

auto main() -> int
{
	try
	{
		benchmark::test_algorithm_correctness();
		benchmark::run_comprehensive_benchmark();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << '\n';
		return 1;
	}

	return 0;
}