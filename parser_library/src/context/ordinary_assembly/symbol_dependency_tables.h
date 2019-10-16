#ifndef SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H
#define SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

#include "address.h"
#include "dependant.h"
#include "dependable.h"
#include "address_resolver.h"
#include "postponed_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class ordinary_assembly_context;


struct statement_ref
{
	using ref_t = std::unordered_set<post_stmt_ptr>::const_iterator;
	statement_ref(ref_t stmt_ref, size_t ref_count = (size_t)1);

	std::unordered_set<post_stmt_ptr>::const_iterator stmt_ref;
	size_t ref_count;
};

class dependency_adder;
//class holding data about dependencies between symbols
class symbol_dependency_tables
{
	//actual dependecies of symbol or space
	std::unordered_map<dependant, const resolvable*> dependencies_;
	//optional length dependency to dependency of symbol
	std::unordered_map<id_index, const resolvable*> length_dependencies_;

	//statement where dependencies are from
	std::unordered_map<dependant, statement_ref> dependency_source_stmts_;
	//list of statements containing dependencies that can not be checked yet
	std::unordered_set<post_stmt_ptr> postponed_stmts_;

	ordinary_assembly_context& sym_ctx_;

	bool check_cycle(dependant target, std::vector<dependant> dependencies);

	void resolve_dependant(dependant target, const resolvable* dep_src);
	void resolve_dependant_default(dependant target);
	void resolve();

	std::vector<dependant> extract_dependencies(const resolvable* dependency_source);
	std::vector<dependant> extract_dependencies(const std::vector<const resolvable*>& dependency_sources);

	void try_erase_source_statement(dependant index);

	bool add_dependency(dependant target, const resolvable* dependency_source, bool check_cycle);
public:

	symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

	//add symbol dependency on statement
	//returns false if cyclic dependency occured
	[[nodiscard]] bool add_dependency(id_index target, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt);

	//add symbol attribute dependency on statement
	//returns false if cyclic dependency occured
	[[nodiscard]] bool add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt);

	//add space dependency
	void add_dependency(space_ptr target, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt);

	//add statement dependency on its operands
	void add_dependency(post_stmt_ptr target);

	//method for creating more than one dependency assigned to one statement
	dependency_adder add_dependencies(post_stmt_ptr dependency_source_stmt);

	//registers that some symbol has been defined
	void add_defined();

	bool check_loctr_cycle();

	//collect all postponed statements either if they still contain dependent objects
	std::vector<post_stmt_ptr> collect_postponed();

	friend dependency_adder;
};

class dependency_adder
{
	symbol_dependency_tables& owner_;
	size_t ref_count_;

	std::vector<dependant> dependants;
public:
	post_stmt_ptr source_stmt;

	dependency_adder(symbol_dependency_tables& owner, post_stmt_ptr dependency_source_stmt);

	[[nodiscard]] bool add_dependency(id_index target, const resolvable* dependency_source);

	[[nodiscard]] bool add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source);

	void add_dependency(space_ptr target, const resolvable* dependency_source);

	void add_dependency();

	void finish();
};

}
}
}

#endif
