#ifndef SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H
#define SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <queue>

#include "address.h"
#include "dependable.h"
#include "../id_generator.h"
#include "address_resolver.h"
#include "postponed_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class ordinary_assembly_context;

enum class dependant_kind { SYMBOL = 0, SPACE = 1 };
struct dependant
{
	dependant(id_index symbol_id);
	dependant(space_ptr space_id);

	bool operator==(const dependant& oth) const;
	dependant_kind kind() const;

	std::variant<id_index, space_ptr> value;
};

struct dependant_hash
{
	std::size_t operator()(const dependant& k) const;
};

//class holding data about dependencies between symbols
class symbol_dependency_tables
{
	//actual dependecies of symbol or space
	std::unordered_map<dependant, const resolvable*, dependant_hash> dependencies_;
	//statement where dependencies are from
	std::unordered_map<dependant, post_stmt_ptr, dependant_hash> dependency_sources_;
	//list of statements containing dependencies that can not be checked yet
	std::vector<std::pair<post_stmt_ptr, std::vector<const resolvable*>>> postponed_stmts_;

	ordinary_assembly_context& sym_ctx_;

	bool check_cycle(dependant target, std::vector<dependant> dependencies);

	void resolve_dependant(dependant target, const resolvable* dep_src);
	void resolve_dependant_default(dependant target);
	void try_resolve(std::queue<dependant> newly_defined);

	std::vector<dependant> extract_dependencies(const resolvable* dependency_source);
	std::vector<dependant> extract_dependencies(const std::vector<const resolvable*>& dependency_sources);
public:

	symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

	//add symbol dependency on statement
	//returns false if cyclic dependency occured
	[[nodiscard]] bool add_dependency(id_index target, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt);

	//add statement dependency on its operands
	void add_dependency(post_stmt_ptr target, std::vector<const resolvable*> dependency_sources);

	//add space dependency
	void add_dependency(space_ptr target, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt);

	//registers that a symbol has been defined
	void add_defined(id_index target);

	bool check_loctr_cycle();

	//registers that a list of symbols have been defined
	void add_defined(const std::vector<id_index>& target);

	//collect all postponed statements either if they still contain dependent objects
	std::vector<post_stmt_ptr> collect_postponed();
};

}
}
}

#endif
