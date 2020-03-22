// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2019, Intel Corporation */

#pragma once

#include "../pmemobj_engine.h"
#include "stree/persistent_b_tree.h"
#include "stree/pstring.h"

using pmem::obj::persistent_ptr;
using pmem::obj::pool;

namespace pmem
{
namespace kv
{
namespace internal
{
namespace stree
{

const size_t DEGREE = 64;
const size_t MAX_KEY_SIZE = 256;
const size_t MAX_VALUE_SIZE = 256;

typedef persistent::b_tree<pstring<MAX_KEY_SIZE>, pstring<MAX_VALUE_SIZE>, DEGREE>
	btree_type;

} /* namespace stree */
} /* namespace internal */

class stree : public pmemobj_engine_base<internal::stree::btree_type> {
public:
	stree(std::unique_ptr<internal::config> cfg);
	~stree();

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	kv_iterator* begin() final;

	kv_iterator* end() final;

	class bidirection_iterator : public kv_iterator {
	public:
		bidirection_iterator();
		explicit bidirection_iterator(internal::stree::btree_type *btree, bool seek_end);
		~bidirection_iterator();
		kv_iterator &operator++() override;
		kv_iterator operator++(int) override;
		kv_iterator &operator--() override;
		kv_iterator operator--(int) override;
		bool operator==(const bidirection_iterator &r);
		bool operator!=(const bidirection_iterator &r);
		string_view operator*() const override;
		string_view key() const override;
		string_view value() const override;
		bool valid();
		void seek_to_first() override;
		void seek_to_last() override;
		void seek(string_view &key) override;
		void seek_for_prev(string_view &key) override;
		void seek_for_next(string_view &key) override;

	private:
		internal::stree::btree_type::iterator m_cur;
		internal::stree::btree_type::iterator m_beg;
		internal::stree::btree_type::iterator m_end;

		internal::stree::btree_type::iterator lower_bound(string_view &key);
		internal::stree::btree_type::iterator upper_bound(string_view &key);
	};

private:
	stree(const stree &);
	void operator=(const stree &);
	void Recover();
	internal::stree::btree_type *my_btree;
};

} /* namespace kv */
} /* namespace pmem */
