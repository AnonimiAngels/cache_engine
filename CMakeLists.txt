cmake_minimum_required(VERSION 3.10)

get_filename_component(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message(STATUS "Project directory: ${PROJECT_DIR}")
project(${PROJECT_DIR})

# Build options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_BENCHMARKS "Build Google Benchmark tests" OFF)
option(ENABLE_PROFILER "Enable built-in profiler" OFF)
option(CACHE_ENGINE_VERBOSE "Enable verbose logging during configuration" OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

function(verbose_message)
	if(CACHE_ENGINE_VERBOSE)
		message(${ARGN})
	endif()
endfunction()

if(NOT DEFINED LLVM_AR)
	find_program(LLVM_AR llvm-ar)
	set(LLVM_AR "${LLVM_AR}" CACHE PATH "Path to LLVM AR")
endif()
if(LLVM_AR)
	verbose_message(STATUS "Using LLVM AR: ${LLVM_AR}")
	set(CMAKE_AR "${LLVM_AR}")
	set(CMAKE_RANLIB "llvm-ranlib")

	set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> --thin rcsu <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> --thin rcsu <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> --thin rcsu <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> --thin rcsu <TARGET> <LINK_FLAGS> <OBJECTS>")
endif()

if(NOT DEFINED MOLD_LINKER)
	find_program(MOLD_LINKER mold)
	set(MOLD_LINKER "${MOLD_LINKER}" CACHE PATH "Path to mold linker")
endif()
if(MOLD_LINKER)
	verbose_message(STATUS "Using mold linker: ${MOLD_LINKER}")
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
		set(CMAKE_LINKER ${MOLD_LINKER})
	else()
		add_link_options("-fuse-ld=mold")
	endif()
endif()

if(NOT DEFINED CCACHE_PROGRAM)
	find_program(CCACHE_PROGRAM ccache)
	set(CCACHE_PROGRAM "${CCACHE_PROGRAM}" CACHE PATH "Path to ccache program")
endif()
if(CCACHE_PROGRAM)
	verbose_message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
	set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
	set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
	set(CMAKE_ASM_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/deps" CACHE PATH "CPM cache directory")
set(CPM_USE_LOCAL_PACKAGES OFF CACHE BOOL "Use local packages when available")
set(CPM_LOCAL_PACKAGES_ONLY OFF CACHE BOOL "Only use local packages")

if(NOT EXISTS "${CPM_SOURCE_CACHE}/cpm")
	verbose_message(STATUS "Downloading CPM.cmake")
	file(DOWNLOAD
		"https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake"
		"${CPM_SOURCE_CACHE}/cpm/CPM.cmake"
	)
endif()
include("${CPM_SOURCE_CACHE}/cpm/CPM.cmake")

# --- Cache Engine Library ---
add_library(cache_engine INTERFACE)
target_include_directories(cache_engine INTERFACE inc)

# --- Dependencies ---
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(cache_engine INTERFACE fmt::fmt spdlog::spdlog)

# --- Testing Dependencies ---
if(BUILD_TESTS)
	verbose_message(STATUS "Configuring Catch2 for testing")

	CPMAddPackage(
		NAME Catch2
		VERSION 2.13.10
		GITHUB_REPOSITORY catchorg/Catch2
		GIT_TAG v2.13.10
		OPTIONS
			"CATCH_INSTALL_DOCS OFF"
			"CATCH_INSTALL_EXTRAS OFF"
	)

	# Ensure Catch2 uses C++11 and create alias for compatibility
	if(TARGET Catch2)
		set_target_properties(Catch2 PROPERTIES
			CXX_STANDARD 11
			CXX_STANDARD_REQUIRED ON
			CXX_EXTENSIONS OFF
		)
		# Create alias for modern target name consistency
		if(NOT TARGET Catch2::Catch2)
			add_library(Catch2::Catch2 ALIAS Catch2)
		endif()
	endif()
endif()

# --- Google Benchmark Integration ---
if(BUILD_BENCHMARKS)
	verbose_message(STATUS "Configuring Google Benchmark integration")

	# First try to find system-installed Google Benchmark
	find_package(benchmark QUIET)

	if(NOT benchmark_FOUND)
		verbose_message(STATUS "System Google Benchmark not found, setting up CPM-based integration")

		# Configure Google Benchmark for C++11 compatibility and project requirements
		set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library" FORCE)
		set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Enable installation of benchmark" FORCE)
		set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Enable building the unit tests which depend on gtest" FORCE)
		set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "Enable building the assembly tests" FORCE)
		set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Allow the downloading and in-tree building of unmet dependencies" FORCE)
		set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "Build Release candidates with -Werror" FORCE)
		set(BENCHMARK_USE_LIBCXX OFF CACHE BOOL "Build benchmark with libc++" FORCE)

		# CPM integration setup for Google Benchmark
		# This will be activated when benchmark source files are added
		set(GOOGLE_BENCHMARK_VERSION "1.8.3" CACHE STRING "Google Benchmark version to use")
		set(GOOGLE_BENCHMARK_REPOSITORY "google/benchmark" CACHE STRING "Google Benchmark repository")

		verbose_message(STATUS "Google Benchmark CPM integration configured (version: ${GOOGLE_BENCHMARK_VERSION})")
		verbose_message(STATUS "Integration will fetch from: https://github.com/${GOOGLE_BENCHMARK_REPOSITORY}")
		verbose_message(STATUS "To activate: Add benchmark source files to benchmarks/google/")

		# Mark integration as ready for directory setup
		set(BENCHMARK_INTEGRATION_READY TRUE)
	else()
		verbose_message(STATUS "Using system-installed Google Benchmark")
		set(BENCHMARK_INTEGRATION_READY TRUE)
	endif()

	# Add Google Benchmark directory if CMakeLists.txt exists
	if(BENCHMARK_INTEGRATION_READY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/google/CMakeLists.txt")
		add_subdirectory(benchmarks/google)
		verbose_message(STATUS "Google Benchmark directory added to build")
	else()
		verbose_message(STATUS "Google Benchmark infrastructure ready - add .cpp files to benchmarks/google/ to enable")
	endif()

	verbose_message(STATUS "Google Benchmark integration completed")
endif()

# --- Compiler Warnings ---
SET(WARNINGS
	-Werror                    # Treat warnings as errors
	-Wall                       # Enable all warnings
	-Wextra                     # Enable all warnings
	-Wpedantic                  # General warnings
	-Wshadow                    # Warns about shadowed variables
	-Wcast-qual                 # Warns about type qualifiers (const, volatile) being cast away
	-Wcast-align                # Warns about potential performance issues due to type casting
	-Wconversion                # Warns about implicit type conversions that may change the value
	-Wsign-conversion           # Warns about implicit conversions between signed and unsigned types
	-Wdouble-promotion          # Warns when a float is promoted to double
	-Wformat=2                  # Warns about format string issues in printf-like functions
	-Wold-style-cast            # Warns about C-style casts (prefer static_cast, etc.)
	-Wpointer-arith             # Warns about pointer arithmetic
	-Wwrite-strings             # Warns about writing to string literals
	-Wredundant-decls           # Warns about redundant declarations
	-Wmissing-declarations      # Warns about missing function declarations
	-Wunreachable-code          # Warns about unreachable code
	-Wfloat-equal               # Warns about floating point equality comparisons
	-Wformat-security           # Warns about potential security issues in format strings
	-Warray-bounds              # Warns about array bounds violations
	-Wstack-protector           # Warns about functions that should use stack protection
	-Wmissing-field-initializers # Warns about missing field initializers
	-Wuninitialized             # Warns about uninitialized variables
	-Winit-self                 # Warns about variables initialized with themselves
	-Wundef                     # Warns about undefined macros in #if
	-Winline                    # Warns when inline functions cannot be inlined
)

# --- Basic Benchmark ---
if(BUILD_BENCHMARKS)
	add_executable(cache_benchmark benchmarks/main.cpp)
	target_link_libraries(cache_benchmark PRIVATE cache_engine)

# Set compile flags
if(MSVC)
	target_compile_options(cache_benchmark PRIVATE
		$<$<CONFIG:Debug>:/Od /Zi>
		$<$<CONFIG:Release>:/O2 /DNDEBUG>
		$<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
	)
	target_compile_definitions(cache_benchmark PRIVATE
		NOMINMAX
		WIN32_LEAN_AND_MEAN
		_WINDOWS
		_CRT_SECURE_NO_WARNINGS
		_CRT_NONSTDC_NO_DEPRECATE
	)
else()
	target_compile_options(cache_benchmark PRIVATE
		${WARNINGS}
		$<$<CONFIG:Debug>:-g -O0>
		$<$<CONFIG:Release>:-O3 -DNDEBUG>
		$<$<CONFIG:RelWithDebInfo>:-O2 -g>
	)
endif()

# Compiler definitions
if(ENABLE_PROFILER)
	target_compile_definitions(cache_benchmark PUBLIC ENABLE_PROFILER)
endif()

# Windows-specific libraries
if(WIN32)
	target_link_libraries(cache_benchmark PRIVATE dbghelp)
endif()

# Visual Studio specific configuration
if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cache_benchmark)
	set_target_properties(cache_benchmark PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
endif()

	include(cmake/modules/static_analysis.cmake)
	enable_clang_tidy_for_target(cache_benchmark)
endif()

# --- Tests ---
if(BUILD_TESTS)
	enable_testing()

	# Create test executables for each test directory
	file(GLOB_RECURSE UNIT_TEST_SOURCES "tests/unit/*.cpp")
	file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "tests/integration/*.cpp")
	file(GLOB_RECURSE PROPERTY_TEST_SOURCES "tests/property/*.cpp")

	# Unit tests
	if(UNIT_TEST_SOURCES)
		add_executable(unit_tests ${UNIT_TEST_SOURCES})
		target_link_libraries(unit_tests PRIVATE cache_engine Catch2::Catch2)

		# Apply same compiler settings as benchmark
		if(MSVC)
			target_compile_options(unit_tests PRIVATE
				$<$<CONFIG:Debug>:/Od /Zi>
				$<$<CONFIG:Release>:/O2 /DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
			)
			target_compile_definitions(unit_tests PRIVATE
				NOMINMAX
				WIN32_LEAN_AND_MEAN
				_WINDOWS
				_CRT_SECURE_NO_WARNINGS
				_CRT_NONSTDC_NO_DEPRECATE
			)
		else()
			target_compile_options(unit_tests PRIVATE
				${WARNINGS}
				$<$<CONFIG:Debug>:-g -O0>
				$<$<CONFIG:Release>:-O3 -DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:-O2 -g>
			)
		endif()

		enable_clang_tidy_for_target(unit_tests)
		add_test(NAME unit_tests COMMAND unit_tests)
	endif()

	# Integration tests
	if(INTEGRATION_TEST_SOURCES)
		add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
		target_link_libraries(integration_tests PRIVATE cache_engine Catch2::Catch2)

		# Apply same compiler settings as benchmark
		if(MSVC)
			target_compile_options(integration_tests PRIVATE
				$<$<CONFIG:Debug>:/Od /Zi>
				$<$<CONFIG:Release>:/O2 /DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
			)
			target_compile_definitions(integration_tests PRIVATE
				NOMINMAX
				WIN32_LEAN_AND_MEAN
				_WINDOWS
				_CRT_SECURE_NO_WARNINGS
				_CRT_NONSTDC_NO_DEPRECATE
			)
		else()
			target_compile_options(integration_tests PRIVATE
				${WARNINGS}
				$<$<CONFIG:Debug>:-g -O0>
				$<$<CONFIG:Release>:-O3 -DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:-O2 -g>
			)
		endif()

		enable_clang_tidy_for_target(integration_tests)
		add_test(NAME integration_tests COMMAND integration_tests)
	endif()

	# Property tests
	if(PROPERTY_TEST_SOURCES)
		add_executable(property_tests ${PROPERTY_TEST_SOURCES})
		target_link_libraries(property_tests PRIVATE cache_engine Catch2::Catch2)

		# Apply same compiler settings as benchmark
		if(MSVC)
			target_compile_options(property_tests PRIVATE
				$<$<CONFIG:Debug>:/Od /Zi>
				$<$<CONFIG:Release>:/O2 /DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
			)
			target_compile_definitions(property_tests PRIVATE
				NOMINMAX
				WIN32_LEAN_AND_MEAN
				_WINDOWS
				_CRT_SECURE_NO_WARNINGS
				_CRT_NONSTDC_NO_DEPRECATE
			)
		else()
			target_compile_options(property_tests PRIVATE
				${WARNINGS}
				$<$<CONFIG:Debug>:-g -O0>
				$<$<CONFIG:Release>:-O3 -DNDEBUG>
				$<$<CONFIG:RelWithDebInfo>:-O2 -g>
			)
		endif()

		enable_clang_tidy_for_target(property_tests)
		add_test(NAME property_tests COMMAND property_tests)
	endif()

	# Create a combined test target for convenience
	if(UNIT_TEST_SOURCES OR INTEGRATION_TEST_SOURCES OR PROPERTY_TEST_SOURCES)
		add_custom_target(run_all_tests
			COMMAND ${CMAKE_CTEST_COMMAND} --verbose
			DEPENDS
				$<$<BOOL:${UNIT_TEST_SOURCES}>:unit_tests>
				$<$<BOOL:${INTEGRATION_TEST_SOURCES}>:integration_tests>
				$<$<BOOL:${PROPERTY_TEST_SOURCES}>:property_tests>
			COMMENT "Running all tests"
		)
	endif()
endif()

# Summary
verbose_message(STATUS "Cache Engine configuration:")
verbose_message(STATUS "   Build tests: ${BUILD_TESTS}")
verbose_message(STATUS "   Build benchmarks: ${BUILD_BENCHMARKS}")
verbose_message(STATUS "   Enable profiler: ${ENABLE_PROFILER}")
if(BUILD_TESTS)
	verbose_message(STATUS "   Test targets: unit_tests, integration_tests, property_tests, run_all_tests")
endif()
