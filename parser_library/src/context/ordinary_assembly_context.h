#pragma once
#include "symbol.h"
#include "alignment.h"
#include "location_counter.h"
#include "section.h"
#include "symbol_dependency_tables.h"
#include "dependable.h"

#include <unordered_map>


namespace hlasm_plugin {
namespace parser_library {
namespace context {

//class holding complete information about the 'ordinary assembly' (assembler and machine instructions)
//it contains 'sections' ordinary 'symbols' and all dependencies between them
class ordinary_assembly_context : public dependency_solver
{
	std::vector<std::unique_ptr<section>> sections_;
	std::unordered_map<id_index,symbol> symbols_;

	section* curr_section_;
public:
	id_storage& ids;

	const std::vector<std::unique_ptr<section>>& sections() const;

	symbol_dependency_tables symbol_dependencies;

	ordinary_assembly_context(id_storage& storage);

	void create_symbol(id_index name, symbol_value value, symbol_attributes attributes);

	symbol* get_symbol(id_index name) override;

	//void create_literal();

	const section* current_section() const;

	void set_section(id_index name, const section_kind kind);

	void set_location_counter(id_index name);

	bool symbol_defined(id_index name);
	bool section_defined(id_index name, const section_kind kind);
	bool counter_defined(id_index name);

	template<size_t byte, size_t boundary>
	address reserve_storage_area(size_t length, alignment<byte, boundary> algn)
	{
		if (!curr_section_)
			create_private_section();

		return curr_section_->current_location_counter().reserve_storage_area(length, algn);
	}

	template<size_t byte, size_t boundary>
	address align(alignment<byte, boundary> algn)
	{
		if (!curr_section_)
			create_private_section();

		return curr_section_->current_location_counter().align(algn);
	}

	space_ptr register_space();

	void finish_module_layout();

private:
	void create_private_section();

};

}
}
}