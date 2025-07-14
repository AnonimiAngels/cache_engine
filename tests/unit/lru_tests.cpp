#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cache_engine/cache.hpp>
#include <stdexcept>
#include <cstdint>

TEST_CASE("LRU cache basic functionality", "[lru][unit]")
{
	SECTION("Cache creation and basic operations")
	{
		cache_engine::cache<std::int32_t, std::string> lru_cache(2U);
		
		// Test put operation
		REQUIRE_NOTHROW(lru_cache.put(1, "first"));
		REQUIRE_NOTHROW(lru_cache.put(2, "second"));
		
		// Test get operation
		REQUIRE((lru_cache.get(1) == "first"));
		REQUIRE((lru_cache.get(2) == "second"));
	}
	
	SECTION("Cache eviction behavior")
	{
		cache_engine::cache<std::int32_t, std::string> lru_cache(2U);
		
		// Fill cache to capacity
		lru_cache.put(1, "first");
		lru_cache.put(2, "second");
		
		// Access first key to make it most recently used
		REQUIRE((lru_cache.get(1) == "first"));
		
		// Add third key - should evict key 2 (least recently used)
		lru_cache.put(3, "third");
		
		// Key 1 should still be accessible
		REQUIRE((lru_cache.get(1) == "first"));
		// Key 3 should be accessible
		REQUIRE((lru_cache.get(3) == "third"));
		// Key 2 should be evicted
		REQUIRE_THROWS_AS(lru_cache.get(2), std::out_of_range);
	}
	
	SECTION("Key update behavior")
	{
		cache_engine::cache<std::int32_t, std::string> lru_cache(2U);
		
		lru_cache.put(1, "first");
		lru_cache.put(2, "second");
		
		// Update existing key
		lru_cache.put(1, "updated_first");
		
		// Value should be updated
		REQUIRE((lru_cache.get(1) == "updated_first"));
		REQUIRE((lru_cache.get(2) == "second"));
	}
	
	SECTION("Exception handling for missing keys")
	{
		cache_engine::cache<std::int32_t, std::string> lru_cache(2U);
		
		// Getting non-existent key should throw
		REQUIRE_THROWS_AS(lru_cache.get(999), std::out_of_range);
	}
}

TEST_CASE("LRU cache edge cases", "[lru][unit][edge]")
{
	SECTION("Single capacity cache")
	{
		cache_engine::cache<std::int32_t, std::string> lru_cache(1U);
		
		lru_cache.put(1, "first");
		REQUIRE((lru_cache.get(1) == "first"));
		
		// Adding second key should evict first
		lru_cache.put(2, "second");
		REQUIRE((lru_cache.get(2) == "second"));
		REQUIRE_THROWS_AS(lru_cache.get(1), std::out_of_range);
	}
	
	SECTION("Different key and value types")
	{
		cache_engine::cache<std::string, std::int32_t> lru_cache(3U);
		
		lru_cache.put("key1", 100);
		lru_cache.put("key2", 200);
		lru_cache.put("key3", 300);
		
		REQUIRE((lru_cache.get("key1") == 100));
		REQUIRE((lru_cache.get("key2") == 200));
		REQUIRE((lru_cache.get("key3") == 300));
	}
}