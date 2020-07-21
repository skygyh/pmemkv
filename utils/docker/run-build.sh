#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# run-build.sh - is called inside a Docker container,
#                starts pmemkv builds (defined by the CI jobs) with tests.
#

set -e

source `dirname $0`/prepare-for-build.sh

# params set for the file (if not previously set, the right-hand param is used)
TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON}
CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON}
TESTS_LONG=${TESTS_LONG:-OFF}

###############################################################################
# BUILD tests_gcc_debug_cpp11
###############################################################################
function tests_gcc_debug_cpp11() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DCOVERAGE=$COVERAGE \
		-DENGINE_STREE=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DTESTS_LONG=${TESTS_LONG} \
		-DDEVELOPER_MODE=1 \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_PMEMOBJ_DRD_HELGRIND=1 \
		-DCXX_STANDARD=11

	make -j$(nproc)
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck|_pmreorder" --timeout 590 --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov gcc_debug_cpp11
	fi

	cd ..
	rm -r build

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14
###############################################################################
function tests_gcc_debug_cpp14() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DCOVERAGE=$COVERAGE \
		-DENGINE_CSMAP=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DTESTS_LONG=${TESTS_LONG} \
		-DDEVELOPER_MODE=1 \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_PMEMOBJ_DRD_HELGRIND=1 \
		-DCXX_STANDARD=14

	make -j$(nproc)
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck|_pmreorder" --timeout 590 --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov gcc_debug_cpp14
	fi

	cd ..
	rm -r build

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_other
###############################################################################
function tests_gcc_debug_cpp14_valgrind_other() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DCOVERAGE=$COVERAGE \
		-DENGINE_CSMAP=1 \
		-DENGINE_STREE=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DCXX_STANDARD=14

	make -j$(nproc)
	ctest -E "_none|_memcheck|_drd" --timeout 590 --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov gcc_debug_cpp14_valgrind_other
	fi

	cd ..
	rm -r build

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_memcheck_drd
###############################################################################
function tests_gcc_debug_cpp14_valgrind_memcheck_drd() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DCOVERAGE=$COVERAGE \
		-DENGINE_CSMAP=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_PMEMOBJ_DRD_HELGRIND=1 \
		-DCXX_STANDARD=14

	make -j$(nproc)
	ctest -R "_memcheck|_drd" --timeout 590 --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov gcc_debug_cpp14_valgrind_memcheck_drd
	fi

	cd ..
	rm -r build

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_clang_release_cpp20 llvm
###############################################################################
function tests_clang_release_cpp20() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# CXX_STANDARD==20 is supported since CMake 3.12
	if [ $CMAKE_VERSION_NUMBER -lt 312 ]; then
		echo "ERROR: C++20 is supported in CMake since 3.12, installed version: ${CMAKE_VERSION}"
		exit 1
	fi

	mkdir build
	cd build

	CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DCOVERAGE=$COVERAGE \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_PMEMOBJ_DRD_HELGRIND=1 \
		-DDEVELOPER_MODE=1 \
		-DCXX_STANDARD=20

	make -j$(nproc)
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck|_pmreorder" --timeout 590 --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov clang_release_cpp20
	fi

	cd ..
	rm -rf build

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD test_release_installation
###############################################################################
function test_release_installation() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DBUILD_TESTS=0 \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG}

	make -j$(nproc)
	sudo_password -S make -j$(nproc) install

	# Verify installed libraries
	compile_example_standalone pmemkv_basic_c
	run_example_standalone pmemkv_basic_c pool
	compile_example_standalone pmemkv_basic_cpp
	run_example_standalone pmemkv_basic_cpp pool
	if [ "$BUILD_JSON_CONFIG" == "ON" ]; then
		compile_example_standalone pmemkv_config_c
		run_example_standalone pmemkv_config_c pool
	fi
	compile_example_standalone pmemkv_pmemobj_cpp
	run_example_standalone pmemkv_pmemobj_cpp pool

	# Poolset example
	compile_example_standalone pmemkv_open_cpp
	pmempool create -l "pmemkv" obj $WORKDIR/examples/example.poolset
	run_example_standalone pmemkv_open_cpp $WORKDIR/examples/example.poolset

	# Expect failure - non-existing path is passed
	run_example_standalone pmemkv_open_cpp /non-existing/path && exit 1

	# Uninstall libraries
	cd $WORKDIR/build
	sudo_password -S make uninstall

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

# Main:
cd $WORKDIR

echo
echo "### Making sure there is no libpmemkv currently installed"
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone pmemkv_basic_cpp && exit 1
echo "---------------------------------------------------------------------------"

# Run build steps passed as script arguments
build_steps=$@
for build in $build_steps
do
	$build
done
