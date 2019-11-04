#include "ordinary_assembly_context.h"
#include "alignment.h"
#include <algorithm>
#include <stdexcept>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

void ordinary_assembly_context::create_private_section()
{
	sections_.emplace_back(std::make_unique<section>(id_storage::empty_id, section_kind::EXECUTABLE, ids));
	curr_section_ = sections_.back().get();
}

const std::vector<std::unique_ptr<section>>& ordinary_assembly_context::sections() const
{
	return sections_;
}

ordinary_assembly_context::ordinary_assembly_context(id_storage& storage)
	: curr_section_(nullptr), ids(storage), symbol_dependencies(*this) {}

bool ordinary_assembly_context::create_symbol(id_index name, symbol_value value, symbol_attributes attributes, location symbol_location)
{
	auto res = symbols_.try_emplace(name, name, value, attributes, std::move(symbol_location));

	if (!res.second)
		throw std::runtime_error("symbol name in use");

	bool ok = symbol_dependencies.check_loctr_cycle();

	symbol_dependencies.add_defined();

	return ok;
}

symbol* ordinary_assembly_context::get_symbol(id_index name)
{
	auto tmp = symbols_.find(name);

	return tmp == symbols_.end() ? nullptr : &tmp->second;
}

const section* ordinary_assembly_context::current_section() const
{
	return curr_section_;
}

void ordinary_assembly_context::set_section(id_index name, const section_kind kind, location symbol_location)
{
	auto tmp = std::find_if(sections_.begin(), sections_.end(), [&](auto & sect) {return sect->name == name && sect->kind == kind; });

	if (tmp != sections_.end())
		curr_section_ = &**tmp;
	else
	{
		if (symbols_.find(name)!=symbols_.end())
			throw std::invalid_argument("symbol already defined");

		sections_.emplace_back(std::make_unique<section>(name, kind,ids));
		curr_section_ = sections_.back().get();

		auto tmp_addr = curr_section_->current_location_counter().reserve_storage_area(0, no_align);
		symbols_.try_emplace(name, name, tmp_addr, symbol_attributes::make_section_attrs(), std::move(symbol_location));
	}
}

void ordinary_assembly_context::set_location_counter(id_index name, location symbol_location)
{
	if (!curr_section_)
		create_private_section();

	bool defined = curr_section_->counter_defined(name);

	curr_section_->set_location_counter(name);

	if (!defined)
	{
		auto tmp_addr = curr_section_->current_location_counter().reserve_storage_area(0, no_align);

		auto sym_tmp = symbols_.try_emplace(name, name, tmp_addr, symbol_attributes::make_section_attrs(), std::move(symbol_location));
		if (!sym_tmp.second)
			throw std::invalid_argument("symbol already defined");
	}
}

bool ordinary_assembly_context::symbol_defined(id_index name)
{
	return symbols_.find(name) != symbols_.end();
}

bool ordinary_assembly_context::section_defined(id_index name, const section_kind kind)
{
	return std::find_if(sections_.begin(), sections_.end(), [&](auto & sect) {return sect->name == name && sect->kind == kind; }) != sections_.end();
}

bool ordinary_assembly_context::counter_defined(id_index name)
{
	if (!curr_section_)
		create_private_section();
	
	return curr_section_->counter_defined(name);
}

//reserves storage area of specified length and alignment

address ordinary_assembly_context::reserve_storage_area(size_t length, alignment align)
{
	if (!curr_section_)
		create_private_section();

	return curr_section_->current_location_counter().reserve_storage_area(length, align);
}

//aligns storage

address ordinary_assembly_context::align(alignment align)
{
	if (!curr_section_)
		create_private_section();

	return curr_section_->current_location_counter().align(align);
}

space_ptr ordinary_assembly_context::register_space()
{
	if (!curr_section_)
		create_private_section();

	return curr_section_->current_location_counter().register_space();
}

void ordinary_assembly_context::finish_module_layout()
{
	for (auto& sect : sections_)
		sect->finish_layout();
}

const std::unordered_map<id_index, symbol>& ordinary_assembly_context::get_all_symbols()
{
	return symbols_;
}
