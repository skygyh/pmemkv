// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "cmap.h"
#include "../out.h"

#include <unistd.h>

namespace pmem
{
namespace kv
{

cmap::cmap(std::unique_ptr<internal::config> cfg) : pmemobj_engine_base(cfg)
{
	static_assert(
		sizeof(internal::cmap::string_t) == 40,
		"Wrong size of cmap value and key. This probably means that std::string has size > 32");

	LOG("Started ok");
	Recover();
}

cmap::~cmap()
{
	LOG("Stopped ok");
}

std::string cmap::name()
{
	return "cmap";
}

status cmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

status cmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();
	for (auto it = container->begin(); it != container->end(); ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status cmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	return container->count(key) == 1 ? status::OK : status::NOT_FOUND;
}

status cmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::cmap::map_t::const_accessor result;
	bool found = container->find(result, key);
	if (!found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(result->second.c_str(), result->second.size(), arg);
	return status::OK;
}

status cmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	container->insert_or_assign(key, value);

	return status::OK;
}

status cmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	bool erased = container->erase(key);
	return erased ? status::OK : status::NOT_FOUND;
}

status cmap::defrag(double start_percent, double amount_percent)
{
	LOG("defrag: start_percent = " << start_percent
				       << " amount_percent = " << amount_percent);
	check_outside_tx();

	try {
		container->defragment(start_percent, amount_percent);
	} catch (std::range_error &e) {
		out_err_stream("defrag") << e.what();
		return status::INVALID_ARGUMENT;
	} catch (pmem::defrag_error &e) {
		out_err_stream("defrag") << e.what();
		return status::DEFRAG_ERROR;
	}

	return status::OK;
}

void cmap::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		container = (pmem::kv::internal::cmap::map_t *)pmemobj_direct(*root_oid);
		container->runtime_initialize();
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::cmap::map_t>().raw();
			container = (pmem::kv::internal::cmap::map_t *)pmemobj_direct(
				*root_oid);
			container->runtime_initialize();
		});
	}
}

kv_iterator *cmap::begin()
{
	check_outside_tx();
	LOG("Creating begin kv_iterator");
	kv_iterator *pit = new bidirection_iterator(container, false);
	return pit;
}

kv_iterator *cmap::end()
{
	check_outside_tx();
	LOG("Creating end kv_iterator");
	kv_iterator *pit = new bidirection_iterator(container, true);
	return pit;
}

cmap::bidirection_iterator::bidirection_iterator()
{
}

cmap::bidirection_iterator::bidirection_iterator(internal::cmap::map_t * _container,
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

cmap::bidirection_iterator::~bidirection_iterator()
{
}

// Prefix ++ overload
kv_iterator &cmap::bidirection_iterator::operator++()
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
kv_iterator cmap::bidirection_iterator::operator++(int)
{
	kv_iterator old = *this;
	++*this;
	return old;
}

// Prefix -- overload
kv_iterator &cmap::bidirection_iterator::operator--()
{
	throw std::runtime_error("seek_to_last not supported");
	/**
	 * [TODO] reverse iterator is not supported yet by hash_map_iterator. Check more in include/libpmemobj++/container/concurrent_hash_map.hpp
	 * Comment out below lines until it is supported by hash_map_iterator.
	 */
	//if (m_cur == m_beg) {
	//	m_cur = m_end;
	//}
	//else {
	//	--m_cur;
	//}
	//return *this;
}
// Postfix -- overload
kv_iterator cmap::bidirection_iterator::operator--(int)
{
	kv_iterator old = *this;
	--*this;
	return old;
}

bool cmap::bidirection_iterator::operator==(const bidirection_iterator &r)
{
	return m_cur == r.m_cur;
}

bool cmap::bidirection_iterator::operator!=(const bidirection_iterator &r)
{
	return !(*this == r);
}

// return key only
string_view cmap::bidirection_iterator::operator*() const
{
	return string_view((*m_cur).first.c_str());
}

string_view cmap::bidirection_iterator::key() const
{
	return string_view((*m_cur).first.c_str());
}

string_view cmap::bidirection_iterator::value() const
{
	return string_view((*m_cur).second.c_str());
}

bool cmap::bidirection_iterator::valid()
{
	return m_cur != m_end;
}

void cmap::bidirection_iterator::seek_to_first()
{
	m_cur = m_beg;
}

void cmap::bidirection_iterator::seek_to_last()
{
	throw std::runtime_error("seek_to_last not supported");
	/**
	 * [TODO] reverse iterator is not supported yet by hash_map_iterator. Check more in include/libpmemobj++/container/concurrent_hash_map.hpp
	 * Comment out below two lines unless it is supported by hash_map_iterator.
	 */
	//m_cur = m_end;
	//--m_cur;
}

void cmap::bidirection_iterator::seek(string_view &key)
{
	for (m_cur = m_beg; m_cur != m_end; ++m_cur) {
		string_view cur_key((*m_cur).first.c_str(), (*m_cur).first.size());
		if (key.compare(cur_key) == 0) {
			break;
		}
	}
}

void cmap::bidirection_iterator::seek_for_prev(string_view &key)
{
	throw std::runtime_error("seek_for_prev not supported");
	/**
	 * [TODO] reverse iterator is not supported yet by hash_map_iterator. Check more in include/libpmemobj++/container/concurrent_hash_map.hpp
	 * Comment out below lines until it is supported by hash_map_iterator.
	 */
	//seek(key);
	//if (m_cur == m_beg) {
	//	m_cur = m_end;
	//	return;
	//}
	//--m_cur;
}

void cmap::bidirection_iterator::seek_for_next(string_view &key)
{
	seek(key);
	if (m_cur == m_end) {
		return;
	}
	++m_cur;
}

} // namespace kv
} // namespace pmem
