#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cache_engine/cache.hpp>
#include <string>
#include <cstdint>

TEST_CASE("Cache algorithm comparison", "[integration][algorithms]")
{
	const std::size_t cache_capacity = 3U;
	
	SECTION("Different algorithms handle same operations")
	{
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::lru> lru_cache(cache_capacity);
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::fifo> fifo_cache(cache_capacity);
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::lfu> lfu_cache(cache_capacity);
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::mfu> mfu_cache(cache_capacity);
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::mru> mru_cache(cache_capacity);
		cache_engine::cache<std::int32_t, std::string, cache_engine::algorithm::random_cache> random_cache(cache_capacity);
		
		// All caches should handle basic put/get operations
		REQUIRE_NOTHROW(lru_cache.put(1, "test"));
		REQUIRE_NOTHROW(fifo_cache.put(1, "test"));
		REQUIRE_NOTHROW(lfu_cache.put(1, "test"));
		REQUIRE_NOTHROW(mfu_cache.put(1, "test"));
		REQUIRE_NOTHROW(mru_cache.put(1, "test"));
		REQUIRE_NOTHROW(random_cache.put(1, "test"));
		
		REQUIRE((lru_cache.get(1) == "test"));
		REQUIRE((fifo_cache.get(1) == "test"));
		REQUIRE((lfu_cache.get(1) == "test"));
		REQUIRE((mfu_cache.get(1) == "test"));
		REQUIRE((mru_cache.get(1) == "test"));
		REQUIRE((random_cache.get(1) == "test"));
	}
}

TEST_CASE("Cache memory behavior", "[integration][memory]")
{
	SECTION("Large cache operations")
	{
		const std::size_t large_capacity = 1000U;
		auto large_cache = cache_engine::make_lru_cache<std::int32_t, std::string>(large_capacity);
		
		// Fill cache with many entries
		for (std::int32_t idx_for = 0; idx_for < static_cast<std::int32_t>(large_capacity); ++idx_for)
		{
			large_cache.put(idx_for, "value_" + std::to_string(idx_for));
		}
		
		// Verify some entries exist
		REQUIRE((large_cache.get(0) == "value_0"));
		REQUIRE((large_cache.get(500) == "value_500"));
		REQUIRE((large_cache.get(999) == "value_999"));
		
		// Adding one more should evict oldest
		large_cache.put(1000, "value_1000");
		REQUIRE((large_cache.get(1000) == "value_1000"));
	}
}