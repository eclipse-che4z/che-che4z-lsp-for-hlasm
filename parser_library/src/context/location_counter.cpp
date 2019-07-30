#include "location_counter.h"
#include "section.h"

#include <stdexcept>
#include <assert.h>
#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

const space_storage& location_counter::spaces() const
{
	return spaces_;
}

size_t location_counter::storage() const
{
	return storage_;
}

location_counter::location_counter(id_index name, const section& owner, const loctr_kind kind, id_storage& ids)
	:  storage_(0),  last_space_(0), ids_(ids), layuot_created_(false), name(name),owner(owner),kind(kind)
{
	if (kind == loctr_kind::NONSTARTING)
	{
		auto id = ids_.add(
			"B " + *owner.name +
			" " + *name +
			" " 
		);

		spaces_.push_back(std::make_shared<space>(*this, id));
	}
}

space_ptr location_counter::register_space()
{
	auto id = ids_.add(
		"S " + *owner.name +
		" " + *name +
		" " + std::to_string(last_space_)
	);

	++last_space_;

	spaces_.push_back(std::make_shared<space>(*this, id));

	return spaces_.back();
}

bool location_counter::has_undefined_layout() const
{
	assert(!(kind == loctr_kind::NONSTARTING) || (*spaces_[0]->name)[0] == 'B');

	if (kind == loctr_kind::STARTING)
		return spaces_.size() > 0;
	else if (kind == loctr_kind::NONSTARTING)
		return spaces_.size() > 1;
	else
	{
		assert(false);
		return true;
	}
}

void location_counter::finish_layout(size_t offset)
{
	if (layuot_created_)
		throw std::runtime_error("layout already created");

	assert(!(kind == loctr_kind::STARTING) || offset == 0); // (STARTING => offset==0) <=> (!STARTING v offset==0)

	assert(!(kind == loctr_kind::NONSTARTING) || (*spaces_[0]->name)[0] == 'B');

	if (kind == loctr_kind::NONSTARTING)
		space::resolve(spaces_[0], offset);

	layuot_created_ = true;
}
