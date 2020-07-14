
// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"
#include "../../src/engines-experimental/stree.h"

using namespace pmem::kv;

const size_t SIZE = 1024ull * 1024ull * 512ull;

static void generateConf(config &cfg)
{
    std::string PATH = "./stree_test";
    std::remove(PATH.c_str());
    auto cfg_s = cfg.put_string("path", PATH);

    if (cfg_s != status::OK)
        throw std::runtime_error("putting 'path' to config failed");

    cfg_s = cfg.put_uint64("force_create", 1);
    if (cfg_s != status::OK)
        throw std::runtime_error("putting 'force_create' to config failed");

    cfg_s = cfg.put_int64("size", SIZE);
    if (cfg_s != status::OK)
        throw std::runtime_error("putting 'size' to config failed");
}

static void StreeFloorAndCeilingEntryTest()
{
    config cfg;
    generateConf(cfg);
    auto kv = INITIALIZE_KV("stree", std::move(cfg));

    // Case 1: Empty DB
    std::size_t cnt = std::numeric_limits<std::size_t>::max();
    UT_ASSERT(kv.count_all(cnt) == status::OK);
    UT_ASSERTeq(cnt, 0);
    UT_ASSERT(kv.get_floor_entry("tmpkey",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
        }, nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_lower_entry("tmpkey",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
        }, nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_ceiling_entry("tmpkey",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
        }, nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_higher_entry("tmpkey",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
        }, nullptr) == status::NOT_FOUND);

    // Case 2: Only one Key "X" in DB
    // The query keys are <, =, > the key "X".
    UT_ASSERT(kv.put("X", "1") == status::OK);
    UT_ASSERT(kv.count_all(cnt) == status::OK);
    UT_ASSERTeq(cnt, 1);
    std::string result;
    UT_ASSERT(kv.get_floor_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    UT_ASSERT(kv.get_lower_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    UT_ASSERT(kv.get_higher_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);

    result.clear();
    UT_ASSERT(kv.get_floor_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_lower_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    UT_ASSERT(kv.get_ceiling_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_higher_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);

    UT_ASSERT(kv.get_floor_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_lower_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_higher_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");

    // Case 3: Two keys "X" and "Y" in DB
    // The query keys are:
    //     3.1 < "X"
    //     3.2 = "X"
    //     3.3 > "X" and < "Y"
    //     3.4 = "Y"
    //     3.5 > "Y"
    UT_ASSERT(kv.put("Y", "2") == status::OK);
    UT_ASSERT(kv.count_all(cnt) == status::OK);
    UT_ASSERTeq(cnt, 2);

    UT_ASSERT(kv.get_floor_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_lower_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_higher_entry("W",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");

    result.clear();
    UT_ASSERT(kv.get_floor_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    UT_ASSERT(kv.get_lower_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_higher_entry("X",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");

    result.clear();
    UT_ASSERT(kv.get_floor_entry("XY",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_lower_entry("XY",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("XY",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");
    result.clear();
    UT_ASSERT(kv.get_higher_entry("XY",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");

    result.clear();
    UT_ASSERT(kv.get_floor_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");
    result.clear();
    UT_ASSERT(kv.get_lower_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<X>,<1>");
    result.clear();
    UT_ASSERT(kv.get_ceiling_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");
    UT_ASSERT(kv.get_higher_entry("Y",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);

    result.clear();
    UT_ASSERT(kv.get_floor_entry("Z",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");
    result.clear();
    UT_ASSERT(kv.get_lower_entry("Z",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
	    const auto c = ((std::string *)arg);
	    c->append("<");
	    c->append(std::string(k, kb));
	    c->append(">,<");
	    c->append(std::string(v, vb));
	    c->append(">");

	    return 0;
	},
	&result) == status::OK);
    UT_ASSERT(result == "<Y>,<2>");
    UT_ASSERT(kv.get_ceiling_entry("Z",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);
    UT_ASSERT(kv.get_higher_entry("Z",
        [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
            UT_ASSERT(false);
            return 0;
	},
	nullptr) == status::NOT_FOUND);

    CLEAR_KV(kv);
    UT_ASSERT(kv.count_all(cnt) == status::OK);
    UT_ASSERTeq(cnt, 0);
    // Case 4: More than one stree::DEGREE keys in DB
    const size_t NUM = internal::stree::DEGREE * 3;
    for (size_t i = 0; i < NUM; ++i) {
        std::string key = std::to_string(i);
	UT_ASSERT(kv.put(key, key) == status::OK);
    }
    UT_ASSERT(kv.count_all(cnt) == status::OK);
    UT_ASSERTeq(cnt, NUM);

    std::srand(std::time(nullptr));
    for (size_t i = 0; i < NUM; ++i) {
        std::string key = std::to_string(size_t(std::rand()) % NUM);

	result.clear();
	UT_ASSERT(kv.get_floor_entry(key,
            [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
                const auto c = ((std::string *)arg);
		c->append("<");
		c->append(std::string(k, kb));
		c->append(">,<");
		c->append(std::string(v, vb));
		c->append(">");

		return 0;
	    },
	    &result) == status::OK);
        std::string expectResult;
        expectResult.append("<");
        expectResult.append(key);
        expectResult.append(">,<");
        expectResult.append(key);
        expectResult.append(">");
	UT_ASSERT(result == expectResult);
	result.clear();
	UT_ASSERT(kv.get_ceiling_entry(key,
            [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
                const auto c = ((std::string *)arg);
		c->append("<");
		c->append(std::string(k, kb));
		c->append(">,<");
		c->append(std::string(v, vb));
		c->append(">");

		return 0;
	    },
	    &result) == status::OK);
	UT_ASSERT(result == expectResult);
    }

    CLEAR_KV(kv);
    kv.close();
}

int main(int argc, char *argv[])
{
    return run_test([&] {
        StreeFloorAndCeilingEntryTest();
    });
}
