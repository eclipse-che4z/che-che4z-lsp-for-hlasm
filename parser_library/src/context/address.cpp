#include "address.h"
#include "location_counter.h"
#include "section.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>

using namespace hlasm_plugin::parser_library::context;

space::space(location_counter& owner, id_index name)
	: name(name), resolved_(false), owner_(owner) {}

void space::resolve(space_ptr this_space, size_t length)
{
	if (this_space->resolved_)
		throw std::invalid_argument("space already resolved");

	auto tmp = std::find_if(this_space->owner_.spaces_.begin(), this_space->owner_.spaces_.end(), [&](auto s) {return s->name == this_space->name; });

	assert(tmp != this_space->owner_.spaces_.end());

	this_space->owner_.storage_ += length;

	this_space->owner_.spaces_.erase(tmp);

	for (auto& listener : this_space->listeners_)
	{
		auto l_tmp = std::find_if(listener->spaces.begin(), listener->spaces.end(), [&](auto s) {return s.first->name == this_space->name; });

		assert(l_tmp != listener->spaces.end());

		listener->offset += length;

		listener->spaces.erase(l_tmp);
	}

	this_space->resolved_ = true;
}

void space::add_listener(address* addr)
{
	assert(std::find(listeners_.begin(), listeners_.end(), addr) == listeners_.end());
	assert(std::find_if(addr->spaces.begin(), addr->spaces.end(), [=](auto& sp) {return &*sp.first == this; }) != addr->spaces.end());
	listeners_.push_back(addr);
}

void space::remove_listener(address* addr)
{
	auto it = std::find(listeners_.begin(), listeners_.end(), addr);
	//assert(std::find_if(addr->spaces.begin(), addr->spaces.end(), [=](auto& sp) {return &*sp.first == this; }) != addr->spaces.end());
	assert(it != listeners_.end());
	listeners_.erase(it);
}

address::address(base address_base, size_t offset, const space_storage& spaces)
	: offset(offset)
{
	bases.emplace_back(address_base, 1);

	for (auto& space : spaces)
	{
		this->spaces.emplace_back(space, 1);
		space->add_listener(this);
	}
}

address::address(const address& addr)
	: address(addr.bases, addr.offset, addr.spaces) {}

address& address::operator=(const address& addr)
{
	for (auto& [sp, count] : spaces)
		sp->remove_listener(this);

	bases = addr.bases;
	offset = addr.offset;
	spaces = addr.spaces;

	for (auto& [sp, count] : spaces)
		sp->add_listener(this);

	return *this;
}

address::address(address&& addr)
	:bases(std::move(addr.bases)),offset(addr.offset),spaces(std::move(addr.spaces))
{
	for (auto& [sp, count] : spaces)
	{
		sp->add_listener(this);
		sp->remove_listener(&addr);
	}

	addr.spaces = {};
}

address& address::operator=(address&& addr)
{
	for (auto& [sp, count] : spaces)
		sp->remove_listener(this);

	bases = std::move(addr.bases);
	offset = std::move(addr.offset);
	spaces = std::move(addr.spaces);

	for (auto& [sp, count] : spaces)
	{
		sp->add_listener(this);
		sp->remove_listener(&addr);
	}

	addr.spaces = {};

	return *this;
}

enum class op { ADD, SUB };

template<typename T>
bool compare(const T& lhs, const T& rhs) { return lhs==rhs; }
template<>
bool compare<space_ptr>(const space_ptr& lhs, const space_ptr& rhs) { return lhs->name==rhs->name; }
template<>
bool compare<address::base>(const address::base& lhs, const address::base& rhs) { return lhs.owner == rhs.owner; }

template <typename T>
std::vector<T> merge_entries(const std::vector<T>& lhs, const std::vector<T>& rhs, const op operation)
{
	std::vector<T> res;

	for (auto& entry : lhs)
	{
		auto it = std::find_if(rhs.begin(), rhs.end(), [&](auto e) {return compare(entry.first, e.first); });

		if (it != rhs.end())
		{
			int count;
			if (operation == op::ADD)
				count = it->second + entry.second;
			else
				count = it->second - entry.second;

			if (count != 0)
				res.emplace_back(entry.first, count);
		}
		else
		{
			res.push_back(entry);
		}
	}

	return res;
}

address address::operator+(const address& addr) const
{
	return address(merge_entries(bases,addr.bases, op::ADD), offset + addr.offset, merge_entries(spaces,addr.spaces, op::ADD));
}

address address::operator+(int offs) const
{
	return address(bases, offset + offs, spaces);
}

address address::operator-(const address& addr) const
{
	return address(merge_entries(bases, addr.bases, op::SUB), offset - addr.offset, merge_entries(spaces, addr.spaces, op::SUB));
}

address address::operator-(int offs) const
{
	return address(bases, offset - offs, spaces);
}

address address::operator-() const
{
	return address(merge_entries({}, bases, op::SUB), -offset, merge_entries({}, spaces, op::SUB));
}

hlasm_plugin::parser_library::context::address::~address()
{
	for (auto&[sp,count] : spaces)
		sp->remove_listener(this);
}

address::address(std::vector<base_entry> bases_, int offset_, std::vector<space_entry> spaces_)
	:bases(std::move(bases_)), offset(offset_), spaces(std::move(spaces_))
{
	for (auto& [sp,count] : spaces)
	{
		assert(count != 0);
		sp->add_listener(this);
	}
}