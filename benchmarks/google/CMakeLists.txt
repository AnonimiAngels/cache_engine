# Google Benchmark CMakeLists.txt for Cache Engine
# This directory will contain Google Benchmark-based performance tests

cmake_minimum_required(VERSION 3.10)

# Set C++11 standard for all benchmark targets
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch Google Benchmark using CPM if not already available
if(NOT TARGET benchmark::benchmark AND NOT TARGET benchmark)
	message(STATUS "Fetching Google Benchmark via CPM")
	
	# Use CPM to fetch Google Benchmark with C++11 compatibility settings
	CPMAddPackage(
		NAME benchmark
		GITHUB_REPOSITORY ${GOOGLE_BENCHMARK_REPOSITORY}
		VERSION ${GOOGLE_BENCHMARK_VERSION}
		OPTIONS
			"BENCHMARK_ENABLE_TESTING OFF"
			"BENCHMARK_ENABLE_INSTALL OFF"
			"BENCHMARK_ENABLE_GTEST_TESTS OFF"
			"BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF"
			"BENCHMARK_DOWNLOAD_DEPENDENCIES OFF"
			"BENCHMARK_ENABLE_WERROR OFF"
			"BENCHMARK_USE_LIBCXX OFF"
	)
	
	if(TARGET benchmark::benchmark)
		message(STATUS "Google Benchmark successfully fetched and configured")
	else()
		message(WARNING "Failed to fetch Google Benchmark - benchmark targets will not be available")
		return()
	endif()
else()
	message(STATUS "Using existing Google Benchmark target")
endif()

# Compiler warnings for benchmark targets
set(BENCHMARK_WARNINGS
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wcast-qual
	-Wcast-align
	-Wconversion
	-Wsign-conversion
	-Wdouble-promotion
	-Wformat=2
	-Wold-style-cast
	-Wpointer-arith
	-Wwrite-strings
	-Wredundant-decls
	-Wmissing-declarations
	-Wunreachable-code
	-Wfloat-equal
	-Wformat-security
	-Warray-bounds
	-Wstack-protector
	-Wmissing-field-initializers
	-Wuninitialized
	-Winit-self
	-Wundef
)

# Function to create benchmark targets
function(add_cache_benchmark target_name source_file)
	add_executable(${target_name} ${source_file})
	
	# Link against cache_engine and Google Benchmark
	# Support both benchmark::benchmark (modern) and benchmark (older versions)
	if(TARGET benchmark::benchmark)
		target_link_libraries(${target_name} PRIVATE cache_engine benchmark::benchmark)
	elseif(TARGET benchmark)
		target_link_libraries(${target_name} PRIVATE cache_engine benchmark)
	else()
		message(FATAL_ERROR "No Google Benchmark target available for ${target_name}")
	endif()
	
	# Set compile options
	if(MSVC)
		target_compile_options(${target_name} PRIVATE
			$<$<CONFIG:Debug>:/Od /Zi>
			$<$<CONFIG:Release>:/O2 /DNDEBUG>
			$<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
		)
		target_compile_definitions(${target_name} PRIVATE
			NOMINMAX
			WIN32_LEAN_AND_MEAN
			_WINDOWS
			_CRT_SECURE_NO_WARNINGS
			_CRT_NONSTDC_NO_DEPRECATE
		)
	else()
		target_compile_options(${target_name} PRIVATE
			${BENCHMARK_WARNINGS}
			$<$<CONFIG:Debug>:-g -O0>
			$<$<CONFIG:Release>:-O3 -DNDEBUG>
			$<$<CONFIG:RelWithDebInfo>:-O2 -g>
		)
	endif()
	
	# Windows-specific libraries
	if(WIN32)
		target_link_libraries(${target_name} PRIVATE dbghelp)
	endif()
	
	# Enable clang-tidy for benchmark targets
	if(COMMAND enable_clang_tidy_for_target)
		enable_clang_tidy_for_target(${target_name})
	endif()
endfunction()

# Register all benchmark executables
add_cache_benchmark(cache_performance_benchmark cache_performance.cpp)
add_cache_benchmark(algorithm_comparison_benchmark algorithm_comparison.cpp)
add_cache_benchmark(memory_efficiency_benchmark memory_efficiency.cpp)
add_cache_benchmark(scaling_analysis_benchmark scaling_analysis.cpp)
add_cache_benchmark(regression_tests_benchmark regression_tests.cpp)

message(STATUS "Google Benchmark directory configured for Cache Engine")