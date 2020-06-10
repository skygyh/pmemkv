// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmemkv_comparator.c -- example usage of pmemkv with custom comparator.
 */

#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LOG(msg) puts(msg)

static const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int get_kv_callback(const char *k, size_t kb, const char *value, size_t value_bytes,
		    void *arg)
{
	printf("   visited: %s\n", k);

	return 0;
}

int reverse_three_way_compare(const char *key1, size_t keybytes1, const char *key2,
			      size_t keybytes2, void *arg)
{
	size_t m_l = MIN(keybytes1, keybytes2);

	int r = memcmp(key2, key1, m_l);
	if (r == 0) {
		if (keybytes2 < keybytes1)
			return -1;
		else if (keybytes2 > keybytes1)
			return 1;
		else
			return 0;
	}

	return r;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	assert(cfg != NULL);

	int s = pmemkv_config_put_string(cfg, "path", argv[1]);
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_uint64(cfg, "size", SIZE);
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_uint64(cfg, "force_create", 1);
	assert(s == PMEMKV_STATUS_OK);

	pmemkv_comparator *cmp = pmemkv_comparator_new(&reverse_three_way_compare,
						       "reverse_three_way_compare", NULL);
	assert(cmp != NULL);
	s = pmemkv_config_put_object(cfg, "comparator", cmp,
				     (void (*)(void *)) & pmemkv_comparator_delete);
	assert(s == PMEMKV_STATUS_OK);

	LOG("Opening pmemkv database with 'csmap' engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("csmap", cfg, &db);
	assert(s == PMEMKV_STATUS_OK);
	assert(db != NULL);

	LOG("Putting new keys");
	const char *key1 = "key1";
	const char *value1 = "value1";
	const char *key2 = "key2";
	const char *value2 = "value2";
	const char *key3 = "key3";
	const char *value3 = "value3";
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_put(db, key2, strlen(key2), value2, strlen(value2));
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_put(db, key3, strlen(key3), value3, strlen(value3));
	assert(s == PMEMKV_STATUS_OK);

	LOG("Iterating over existing keys in order specified by the comparator");
	s = pmemkv_get_all(db, &get_kv_callback, NULL);
	assert(s == PMEMKV_STATUS_OK);

	LOG("Closing database");
	pmemkv_close(db);

	return 0;
}
