#ifndef SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H
#define SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <queue>

#include "address.h"
#include "dependable.h"
#include "id_generator.h"
#include "address_resolver.h"
#include "postponed_statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class ordinary_assembly_context;

//class holding data about dependencies between symbols
class symbol_dependency_tables
{
	std::unordered_map<id_index, std::vector<id_index>> dependencies_;
	std::unordered_map<id_index, std::pair<const std::vector<const resolvable*>, post_stmt_ptr>> dependency_sources_;

	std::vector<space_ptr> spaces_;
	std::vector<std::unique_ptr<address_resolver>> resolvers_;

	std::vector<post_stmt_ptr> ready_to_check_;
	ordinary_assembly_context& sym_ctx_;
	id_generator generator_;

	bool check_cycle(id_index target, const std::vector<id_index>& dependencies);
	void try_resolve(std::queue<id_index> newly_defined);
public:

	symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

	[[nodiscard]] bool add_dependency(id_index target,const std::vector<const resolvable*> dependent, post_stmt_ptr stmt);

	[[nodiscard]] bool add_dependency(id_index target, address dependent, post_stmt_ptr stmt);

	[[nodiscard]] bool add_dependency(space_ptr space, const resolvable* dependent, post_stmt_ptr stmt);

	void add_defined(id_index target);

	void add_defined(const std::vector<id_index>& target);

	std::vector<post_stmt_ptr> collect_unchecked();

	std::vector<post_stmt_ptr> collect_all();

};

}
}
}

#endif
