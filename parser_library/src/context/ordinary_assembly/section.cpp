#include "section.h"

#include <algorithm>
#include <stdexcept>

using namespace hlasm_plugin::parser_library::context;

const std::vector<loctr_ptr>& section::location_counters() const
{
	return loctrs_;
}

section::section(id_index name, const section_kind kind, id_storage& ids)
	: ids_(ids), name(name), kind(kind)
{
	loctrs_.emplace_back(std::make_unique<location_counter>(name, *this, loctr_kind::STARTING, ids_));
	curr_loctr_ = loctrs_.back().get();
}

void section::set_location_counter(id_index loctr_name)
{
	auto tmp = std::find_if(loctrs_.begin(), loctrs_.end(), [&](auto & loctr) {return loctr->name == loctr_name; });

	if (tmp != loctrs_.end())
		curr_loctr_ = &**tmp;
	else
	{
		loctrs_.emplace_back(std::make_unique<location_counter>(loctr_name, *this, loctr_kind::NONSTARTING, ids_));
		curr_loctr_ = loctrs_.back().get();
	}
}

bool section::counter_defined(id_index loctr_name)
{
	return std::find_if(loctrs_.begin(), loctrs_.end(), [&](auto & loctr) {return loctr->name == loctr_name; }) != loctrs_.end();
}

location_counter& section::current_location_counter() const
{
	return *curr_loctr_;
}

void section::finish_layout()
{
	for (size_t i = 0; i < loctrs_.size(); i++)
	{
		if (i == 0)
			loctrs_[0]->finish_layout(0);
		else
		{
			if (!loctrs_[i - 1]->spaces().empty())
				throw std::runtime_error("layout can not be created");

			loctrs_[i]->finish_layout(loctrs_[i - 1]->storage());
		}
	}
}
