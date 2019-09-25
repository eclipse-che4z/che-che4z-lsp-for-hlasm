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
	//list of visited sections
	std::vector<std::unique_ptr<section>> sections_;
	//list of visited symbols
	std::unordered_map<id_index,symbol> symbols_;

	section* curr_section_;
public:
	//access id storage
	id_storage& ids;

	//access sections
	const std::vector<std::unique_ptr<section>>& sections() const;

	//access symbol dependency table
	symbol_dependency_tables symbol_dependencies;

	ordinary_assembly_context(id_storage& storage);

	//creates symbol
	//returns false if loctr cycle has occured
	[[nodiscard]] bool create_symbol(id_index name, symbol_value value, symbol_attributes attributes);

	//gets symbol by name
	virtual symbol* get_symbol(id_index name) override;

	//access current section
	const section* current_section() const;

	//sets current section
	void set_section(id_index name, const section_kind kind);

	//sets current location counter of current section
	void set_location_counter(id_index name);

	//check whether symbol is already defined
	bool symbol_defined(id_index name);
	//check whether section is already defined
	bool section_defined(id_index name, const section_kind kind);
	//check whether location counter is already defined
	bool counter_defined(id_index name);

	//reserves storage area of specified length and alignment
	address reserve_storage_area(size_t length, alignment align);

	//aligns storage
	address align(alignment align);

	//adds space to the current location counter
	space_ptr register_space();

	//creates layout of every section
	void finish_module_layout();

private:
	void create_private_section();
};

}
}
}
