# ============================================================================
# Static Analysis Module
# ============================================================================
# This module handles static analysis tools configuration, primarily Clang-Tidy

# ============================================================================
# Clang-Tidy Configuration
# ============================================================================
# find_program(CLANG_TIDY_EXE NAMES clang-tidy)
# find_program(CLANG_APPLY_REPLACEMENTS_EXE NAMES clang-apply-replacements-19)

# ============================================================================
# Function to enable Clang-Tidy for a target
# ============================================================================
function(enable_clang_tidy_for_target target_name)
	if(CLANG_TIDY_EXE)
		if(EXISTS "${CMAKE_SOURCE_DIR}/.clang-tidy")
			set(CLANG_TIDY_FIXES_FILE "${CMAKE_BINARY_DIR}/clang_tidy_fixes.yaml")
			message(STATUS "Enabling clang-tidy for ${target_name} using .clang-tidy configuration file")

			set_target_properties(${target_name} PROPERTIES
				CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--quiet;--use-color;-export-fixes=${CLANG_TIDY_FIXES_FILE}"
			)

			# Add a custom target for applying clang-tidy fixes
			if(CLANG_APPLY_REPLACEMENTS_EXE)
				# Create a target-specific fix application target
				add_custom_target(apply_clang_tidy_fixes_${target_name}
					COMMAND ${CLANG_APPLY_REPLACEMENTS_EXE} ${CMAKE_BINARY_DIR}
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					COMMENT "Applying clang-tidy fixes for ${target_name}"
					VERBATIM
				)

				# Optionally make fixes dependent on the target
				# add_dependencies(${target_name} apply_clang_tidy_fixes_${target_name})

				message(STATUS "Clang-apply-replacements available - use 'make apply_clang_tidy_fixes_${target_name}' to apply fixes")
			else()
				message(WARNING "clang-apply-replacements not found - cannot apply clang-tidy fixes automatically for ${target_name}")
			endif()
		else()
			message(STATUS "clang-tidy found but no .clang-tidy file - skipping static analysis for ${target_name}")
		endif()
	else()
		message(STATUS "Clang-tidy not found - skipping static analysis for ${target_name}")
	endif()
endfunction()

# ============================================================================
# Clang-Tidy Global Configuration
# ============================================================================
if(FALSE) # Disabled by default - enable by changing to TRUE
	message(STATUS "Global Clang-Tidy analysis is DISABLED")
	message(STATUS "To enable: change FALSE to TRUE in cmake/static_analysis.cmake")
	message(STATUS "Or use enable_clang_tidy_for_target() function for specific targets")
else()
	message(STATUS "Global Clang-Tidy analysis is DISABLED by default")
endif()

# ============================================================================
# Additional Static Analysis Tools
# ============================================================================

# Function to add other static analysis tools
function(add_static_analysis_tools target_name)
	# Placeholder for other static analysis tools
	# e.g., cppcheck, include-what-you-use, etc.

	# Example for cppcheck (commented out)
	# find_program(CPPCHECK_EXE NAMES cppcheck)
	# if(CPPCHECK_EXE)
	#     message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
	#     set(CPPCHECK_ARGS
	#         --enable=all
	#         --std=c++23
	#         --verbose
	#         --quiet
	#         --error-exitcode=1
	#     )
	#     add_custom_target(cppcheck_${target_name}
	#         COMMAND ${CPPCHECK_EXE} ${CPPCHECK_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/src
	#         COMMENT "Running cppcheck on ${target_name}"
	#         VERBATIM
	#     )
	# endif()

	message(STATUS "Static analysis tools configuration completed for ${target_name}")
endfunction()

# ============================================================================
# Static Analysis Summary
# ============================================================================
message(STATUS "=== Static Analysis Configuration ===")
if(CLANG_TIDY_EXE)
	message(STATUS "Clang-Tidy: FOUND (${CLANG_TIDY_EXE})")
	if(EXISTS "${CMAKE_SOURCE_DIR}/.clang-tidy")
		message(STATUS ".clang-tidy config: FOUND")
	else()
		message(STATUS ".clang-tidy config: NOT FOUND")
	endif()
	if(CLANG_APPLY_REPLACEMENTS_EXE)
		message(STATUS "Clang-apply-replacements: FOUND (${CLANG_APPLY_REPLACEMENTS_EXE})")
	else()
		message(STATUS "Clang-apply-replacements: NOT FOUND")
	endif()
else()
	message(STATUS "Clang-Tidy: NOT FOUND")
endif()
message(STATUS "=====================================")