// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void PutKeysOfDifferentSizesTest(pmem::kv::db &kv)
{
	std::string value;
	ASSERT_STATUS(kv.put("123456789ABCDE", "A"), status::OK);
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 1);
	ASSERT_STATUS(kv.get("123456789ABCDE", &value), status::OK);
	UT_ASSERT(value == "A");

	std::string value2;
	ASSERT_STATUS(kv.put("123456789ABCDEF", "B"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 2);
	ASSERT_STATUS(kv.get("123456789ABCDEF", &value2), status::OK);
	UT_ASSERT(value2 == "B");

	std::string value3;
	ASSERT_STATUS(kv.put("12345678ABCDEFG", "C"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 3);
	ASSERT_STATUS(kv.get("12345678ABCDEFG", &value3), status::OK);
	UT_ASSERT(value3 == "C");

	std::string value4;
	ASSERT_STATUS(kv.put("123456789", "D"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 4);
	ASSERT_STATUS(kv.get("123456789", &value4), status::OK);
	UT_ASSERT(value4 == "D");

	std::string value5;
	ASSERT_STATUS(kv.put("123456789ABCDEFGHI", "E"), status::OK);
	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERT(cnt == 5);
	ASSERT_STATUS(kv.get("123456789ABCDEFGHI", &value5), status::OK);
	UT_ASSERT(value5 == "E");
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {
				 PutKeysOfDifferentSizesTest,
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
