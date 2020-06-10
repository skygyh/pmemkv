# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# ctest_helpers.cmake - helper functions for tests/CMakeLists.txt
#

set(TEST_ROOT_DIR ${PROJECT_SOURCE_DIR}/tests)

set(GLOBAL_TEST_ARGS
	-DLIBPMEMOBJ++_LIBRARY_DIRS=${LIBPMEMOBJ++_LIBRARY_DIRS}
	-DPERL_EXECUTABLE=${PERL_EXECUTABLE}
	-DMATCH_SCRIPT=${PROJECT_SOURCE_DIR}/tests/match
	-DPARENT_DIR=${TEST_DIR}
	-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}
	-DTEST_ROOT_DIR=${TEST_ROOT_DIR})
if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(INCLUDE_DIRS ${LIBPMEMOBJ++_INCLUDE_DIRS} common/ ../src .)
set(LIBS_DIRS ${LIBPMEMOBJ++_LIBRARY_DIRS})

# List of supported Valgrind tracers
set(vg_tracers memcheck helgrind drd pmemcheck)

include_directories(${INCLUDE_DIRS})
link_directories(${LIBS_DIRS})

function(find_gdb)
	execute_process(COMMAND gdb --help
			RESULT_VARIABLE GDB_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(GDB_RET)
		set(GDB_FOUND 0 CACHE INTERNAL "")
		message(WARNING "GDB NOT found, some tests will be skipped")
	else()
		set(GDB_FOUND 1 CACHE INTERNAL "")
	endif()
endfunction()

function(find_pmreorder)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBUNWIND QUIET libunwind)
	else()
		find_package(LIBUNWIND QUIET)
	endif()
	if(NOT LIBUNWIND_FOUND)
		message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
	endif()

	if(NOT WIN32)
		if(VALGRIND_FOUND)
			if ((NOT(PMEMCHECK_VERSION LESS 1.0)) AND PMEMCHECK_VERSION LESS 2.0)
				find_program(PMREORDER names pmreorder HINTS ${LIBPMEMOBJ_PREFIX}/bin)

				if(PMREORDER)
					set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
					set(PMREORDER_SUPPORTED true CACHE INTERNAL "pmreorder support")
				endif()
			else()
				message(STATUS "Pmreorder will not be used. Pmemcheck must be installed in version 1.X")
			endif()
		elseif(TESTS_USE_VALGRIND)
			message(WARNING "Valgrind not found. Valgrind tests will not be performed.")
		endif()
	endif()
endfunction()

function(find_pmempool)
	find_program(PMEMPOOL names pmempool HINTS ${LIBPMEMOBJ_PREFIX}/bin)
	if(PMEMPOOL)
		set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
	else()
		message(FATAL_ERROR "Pmempool not found.")
	endif()
endfunction()

# Function to build test with custom build options (e.g. passing defines) and
# link it with custom library/-ies (supports 'json', 'libpmemobj_cpp' and
# 'dl_libs'). It calls build_test function.
# Usage: build_test_ext(NAME .. SRC_FILES .. .. LIBS .. .. BUILD_OPTIONS .. ..)
function(build_test_ext)
	set(oneValueArgs NAME)
	set(multiValueArgs SRC_FILES LIBS BUILD_OPTIONS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	set(LIBS_TO_LINK "")

	foreach(lib ${TEST_LIBS})
		if("${lib}" STREQUAL "json")
			if(NOT BUILD_JSON_CONFIG)
				return()
			else()
				list(APPEND LIBS_TO_LINK pmemkv_json_config)
				list(APPEND TEST_BUILD_OPTIONS -DJSON_TESTS_SUPPORT)
			endif()
		elseif("${lib}" STREQUAL "libpmemobj_cpp")
			if("${LIBPMEMOBJ++_LIBRARIES}" STREQUAL "")
				return()
			else()
				list(APPEND LIBS_TO_LINK ${LIBPMEMOBJ++_LIBRARIES})
			endif()
		elseif("${lib}" STREQUAL "dl_libs")
			list(APPEND LIBS_TO_LINK ${CMAKE_DL_LIBS})
		endif()
	endforeach()

	build_test(${TEST_NAME} ${TEST_SRC_FILES})
	target_link_libraries(${TEST_NAME} ${LIBS_TO_LINK})
	target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_BUILD_OPTIONS})
endfunction()

function(build_test name)
	# skip posix tests
	# XXX: a WIN32 test will break if used with build_test_ext() func.
	if(${name} MATCHES "posix$" AND WIN32)
		return()
	endif()

	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_executable(${name} ${srcs})
	target_link_libraries(${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} pmemkv test_backtrace)
	if(LIBUNWIND_FOUND)
		target_link_libraries(${name} ${LIBUNWIND_LIBRARIES} ${CMAKE_DL_LIBS})
	endif()
	if(WIN32)
		target_link_libraries(${name} dbghelp)
	endif()

	add_dependencies(tests ${name})
endfunction()

# Configures testcase ${test_name}_${testcase} with ${tracer}
# and cmake_script used to execute test
function(add_testcase executable test_name tracer testcase cmake_script)
	add_test(NAME ${test_name}_${testcase}_${tracer}
			COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${test_name}_${testcase}_${tracer}
			-DTESTCASE=${testcase}
			-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${test_name}
			-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${test_name}_${testcase}_${tracer}
			-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>
			-DTRACER=${tracer}
			-DLONG_TESTS=${LONG_TESTS}
			${ARGN}
			-P ${cmake_script})

	set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
			ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};"
			FAIL_REGULAR_EXPRESSION Sanitizer)

	# XXX: if we use FATAL_ERROR in test.cmake - pmemcheck passes anyway
	# workaround: look for "CMake Error" in output and fail if found
	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				FAIL_REGULAR_EXPRESSION "CMake Error")
	endif()

	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 100)
	elseif(${tracer} IN_LIST vg_tracers)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 50)
	else()
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 10)
	endif()
endfunction()

function(skip_test name message)
	add_test(NAME ${name}_${message}
		COMMAND ${CMAKE_COMMAND} -P ${TEST_ROOT_DIR}/true.cmake)

	set_tests_properties(${name}_${message} PROPERTIES COST 0)
endfunction()

# adds testcase only if tracer is found and target is build, skips otherwise
function(add_test_common executable test_name tracer testcase cmake_script)
	if(${tracer} STREQUAL "")
	    set(tracer none)
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} IN_LIST vg_tracers)
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_VALGRIND")
		endif()
		return()
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_PMEMCHECK_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} STREQUAL "pmemcheck")
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_PMEMCHECK")
		endif()
		return()
	endif()

	if (NOT WIN32 AND (USE_ASAN OR USE_UBSAN) AND ${tracer} IN_LIST vg_tracers)
		skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_SANITIZER_USED")
		return()
	endif()

	# if test was not build
	if (NOT TARGET ${executable})
		message(WARNING "${executable} not build. Skipping.")
		return()
	endif()

	# skip all valgrind tests on windows
	if ((NOT ${tracer} STREQUAL none) AND WIN32)
		return()
	endif()

	if (COVERAGE AND ${tracer} IN_LIST vg_tracers)
		return()
	endif()

	add_testcase(${executable} ${test_name} ${tracer} ${testcase} ${cmake_script} ${ARGN})
endfunction()

# adds testcase with optional SCRIPT and TEST_CASE parameters
function(add_test_generic)
	set(oneValueArgs NAME CASE SCRIPT)
	set(multiValueArgs TRACERS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if("${TEST_SCRIPT}" STREQUAL "")
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_default.cmake)
		else()
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/${TEST_NAME}_${TEST_CASE}.cmake)
		endif()
	else()
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
		endif()
		set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT})
	endif()

	foreach(tracer ${TEST_TRACERS})
		add_test_common(${TEST_NAME} ${TEST_NAME} ${tracer} ${TEST_CASE} ${cmake_script})
	endforeach()
endfunction()

# adds testcase with addtional parameters, required by "engine scenario" tests
function(add_engine_test)
	set(oneValueArgs BINARY ENGINE SCRIPT DB_SIZE)
	set(multiValueArgs TRACERS PARAMS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/engines/${TEST_SCRIPT})

	if("${TEST_DB_SIZE}" STREQUAL "")
		set(TEST_DB_SIZE 104857600) # 100MB
	endif()

	get_filename_component(script_name ${cmake_script} NAME)
	set(parsed_script_name ${script_name})
	string(REGEX REPLACE ".cmake" "" parsed_script_name ${parsed_script_name})

	set(TEST_NAME "${TEST_ENGINE}__${TEST_BINARY}__${parsed_script_name}")
	if(NOT "${TEST_PARAMS}" STREQUAL "")
		string(REPLACE ";" "_" parsed_params "${TEST_PARAMS}")
		set(TEST_NAME "${TEST_NAME}_${parsed_params}")
	endif()

	if(${TEST_ENGINE} STREQUAL "caching")
		# caching tests require lib_acl and memcached included,
		# so we need to link them to test binary itself
		target_link_libraries(${TEST_BINARY} memcached)
		target_link_libraries(${TEST_BINARY} acl_cpp protocol acl)
	endif()

	# Use "|PARAM|" as list separator so that CMake does not expand it
	# when passing to the test script
	string(REPLACE ";" "|PARAM|" raw_params "${TEST_PARAMS}")

	foreach(tracer ${TEST_TRACERS})
		add_test_common(${TEST_BINARY} ${TEST_NAME} ${tracer} 0 ${cmake_script}
			-DENGINE=${TEST_ENGINE}
			-DDB_SIZE=${TEST_DB_SIZE}
			-DRAW_PARAMS=${raw_params})
	endforeach()
endfunction()
