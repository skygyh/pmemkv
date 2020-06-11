// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "csmap.h"
#include "../out.h"

namespace pmem
{
namespace kv
{

csmap::csmap(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

csmap::~csmap()
{
	LOG("Stopped ok");
}

std::string csmap::name()
{
	return "csmap";
}

status csmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

template <typename It>
static std::size_t size(It first, It last)
{
	auto dist = std::distance(first, last);
	assert(dist >= 0);

	return static_cast<std::size_t>(dist);
}

status csmap::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->upper_bound(key);
	auto last = container->end();

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->lower_bound(key);
	auto last = container->end();

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->upper_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->lower_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (container->key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);

		cnt = size(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status csmap::iterate(typename container_type::iterator first,
		      typename container_type::iterator last, get_kv_callback *callback,
		      void *arg)
{
	for (auto it = first; it != last; ++it) {
		shared_node_lock_type lock(it->second.mtx);

		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.val.c_str(), it->second.val.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status csmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->upper_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->lower_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->upper_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->lower_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (container->key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);
		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status csmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	return container->contains(key) ? status::OK : status::NOT_FOUND;
}

status csmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	auto it = container->find(key);
	if (it != container->end()) {
		shared_node_lock_type lock(it->second.mtx);
		callback(it->second.val.c_str(), it->second.val.size(), arg);
		return status::OK;
	}

	LOG("  key not found");
	return status::NOT_FOUND;
}

status csmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto result = container->try_emplace(key, value);

	if (result.second == false) {
		auto &it = result.first;
		unique_node_lock_type lock(it->second.mtx);
		pmem::obj::transaction::run(pmpool, [&] {
			it->second.val.assign(value.data(), value.size());
		});
	}

	return status::OK;
}

status csmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	unique_global_lock_type lock(mtx);
	return container->unsafe_erase(key) > 0 ? status::OK : status::NOT_FOUND;
}

void csmap::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<internal::csmap::pmem_type *>(
			pmemobj_direct(*root_oid));

		container = &pmem_ptr->map;
		container->runtime_initialize();
		container->key_comp().runtime_initialize(
			internal::extract_comparator(*config));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::csmap::pmem_type>()
					.raw();
			auto pmem_ptr = static_cast<internal::csmap::pmem_type *>(
				pmemobj_direct(*root_oid));
			container = &pmem_ptr->map;
			container->runtime_initialize();
			container->key_comp().initialize(
				internal::extract_comparator(*config));
		});
	}
}

kv_iterator *csmap::begin()
{
	check_outside_tx();
	LOG("Creating begin kv_iterator");
	kv_iterator *pit = new bidirection_iterator(container, false);
	return pit;
}

kv_iterator *csmap::end()
{
	check_outside_tx();
	LOG("Creating end kv_iterator");
	kv_iterator *pit = new bidirection_iterator(container, true);
	return pit;
}

csmap::bidirection_iterator::bidirection_iterator()
{
}

csmap::bidirection_iterator::bidirection_iterator(container_type * _container,
	bool seek_end = false)
	: m_beg(_container->begin()), m_end(_container->end())
{
	if (seek_end) {
		m_cur = _container->end();
	}
	else {
		m_cur = _container->begin();
	}
}

csmap::bidirection_iterator::~bidirection_iterator()
{
}

// Prefix ++ overload
kv_iterator &csmap::bidirection_iterator::operator++()
{
	if (m_cur == m_end) {
		m_cur = m_beg;
	}
	else {
		++m_cur;
	}
	return *this;
}

// Postfix ++ overload
kv_iterator csmap::bidirection_iterator::operator++(int)
{
	kv_iterator old = *this;
	++*this;
	return old;
}

// Prefix -- overload
kv_iterator &csmap::bidirection_iterator::operator--()
{
	if (m_cur == m_beg) {
		m_cur = m_end;
	} else {
		--m_cur;
	}
	return *this;
}
// Postfix -- overload
kv_iterator csmap::bidirection_iterator::operator--(int)
{
	kv_iterator old = *this;
	--*this;
	return old;
}

bool csmap::bidirection_iterator::operator==(const bidirection_iterator &r)
{
	return m_cur == r.m_cur;
}

bool csmap::bidirection_iterator::operator!=(const bidirection_iterator &r)
{
	return !(*this == r);
}

// return key only
string_view csmap::bidirection_iterator::operator*() const
{
	return string_view((*m_cur).first.c_str());
}

string_view csmap::bidirection_iterator::key() const
{
	return string_view((*m_cur).first.c_str());
}

string_view csmap::bidirection_iterator::value() const
{
	return string_view((*m_cur).second.c_str());
}

bool csmap::bidirection_iterator::valid()
{
	return m_cur != m_end;
}

void csmap::bidirection_iterator::seek_to_first()
{
	m_cur = m_beg;
}

void csmap::bidirection_iterator::seek_to_last()
{
	m_cur = m_end;
	--m_cur;
}

void csmap::bidirection_iterator::seek(string_view &key)
{
	for (m_cur = m_beg; m_cur != m_end; ++m_cur) {
		string_view cur_key((*m_cur).first.c_str(), (*m_cur).first.size());
		if (key.compare(cur_key) == 0) {
			break;
		}
	}
}

void csmap::bidirection_iterator::seek_for_prev(string_view &key)
{

	seek(key);
	if (m_cur == m_beg) {
		m_cur = m_end;
		return;
	}
	--m_cur;
}

void csmap::bidirection_iterator::seek_for_next(string_view &key)
{
	seek(key);
	if (m_cur == m_end) {
		return;
	}
	++m_cur;
}


} // namespace kv
} // namespace pmem
