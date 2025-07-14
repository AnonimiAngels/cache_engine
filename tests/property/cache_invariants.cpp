#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cache_engine/cache.hpp>
#include <random>
#include <vector>
#include <set>
#include <cstdint>

TEST_CASE("Cache capacity invariant", "[property][invariants]")
{
	SECTION("Cache never exceeds capacity")
	{
		const std::size_t capacity = 10U;
		cache_engine::cache<std::int32_t, std::string> test_cache(capacity);
		
		// Generate random operations
		std::random_device random_device;
		std::mt19937 generator(random_device());
		std::uniform_int_distribution<std::int32_t> key_distribution(1, 100);
		
		// Add more items than capacity
		std::set<std::int32_t> inserted_keys;
		for (std::size_t idx_for = 0U; idx_for < capacity * 2U; ++idx_for)
		{
			std::int32_t key = key_distribution(generator);
			test_cache.put(key, "value_" + std::to_string(key));
			inserted_keys.insert(key);
		}
		
		// Verify that we can access at least some keys
		// (exact number depends on collision patterns)
		std::size_t accessible_count = 0U;
		for (const auto& key : inserted_keys)
		{
			try
			{
				test_cache.get(key);
				++accessible_count;
			}
			catch (const std::out_of_range&)
			{
				// Expected for evicted keys
			}
		}
		
		// Should be able to access at most capacity items
		REQUIRE((accessible_count <= capacity));
		// Should be able to access at least 1 item
		REQUIRE((accessible_count >= 1U));
	}
}

TEST_CASE("Cache operation consistency", "[property][consistency]")
{
	SECTION("Put and get operations are consistent")
	{
		cache_engine::cache<std::int32_t, std::string> test_cache(5U);
		
		// Test with various key-value pairs
		std::vector<std::pair<std::int32_t, std::string>> test_data = {
			{1, "first"},
			{2, "second"},
			{3, "third"},
			{4, "fourth"},
			{5, "fifth"}
		};
		
		// Insert all data
		for (const auto& pair : test_data)
		{
			test_cache.put(pair.first, pair.second);
		}
		
		// Verify all data is accessible
		for (const auto& pair : test_data)
		{
			REQUIRE((test_cache.get(pair.first) == pair.second));
		}
	}
	
	SECTION("Key updates preserve consistency")
	{
		cache_engine::cache<std::int32_t, std::string> test_cache(3U);
		
		test_cache.put(1, "original");
		REQUIRE((test_cache.get(1) == "original"));
		
		// Update the value
		test_cache.put(1, "updated");
		REQUIRE((test_cache.get(1) == "updated"));
		
		// Value should remain updated after other operations
		test_cache.put(2, "second");
		test_cache.put(3, "third");
		REQUIRE((test_cache.get(1) == "updated"));
	}
}