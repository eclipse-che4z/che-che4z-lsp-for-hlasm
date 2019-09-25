#include "symbol_dependency_tables.h"
#include "ordinary_assembly_context.h"

#include <stdexcept>
#include <algorithm>
#include <queue>
#include <stack>

using namespace hlasm_plugin::parser_library::context;

bool symbol_dependency_tables::check_cycle(dependant target, std::vector<dependant> dependencies)
{
	if (std::find(dependencies.begin(), dependencies.end(), target) != dependencies.end()) //dependencies contain target itself
		return false;

	while (!dependencies.empty())
	{
		auto top_dep = std::move(dependencies.back());
		dependencies.pop_back();

		auto it = dependencies_.find(top_dep);
		
		if (it != dependencies_.end())
		{
			auto& [_, dep_src] = *it;
			for (auto && dep : extract_dependencies(dep_src))
			{
				if (dep == target)
				{
					resolve_dependant_default(target);
					return false;
				}
				dependencies.push_back(std::move(dep));
			}
		}
	}

	return true;
}

void symbol_dependency_tables::resolve_dependant(dependant target, const resolvable* dep_src)
{
	symbol_value val = dep_src->resolve(sym_ctx_);

	if (target.kind() == dependant_kind::SPACE)
	{
		int32_t length = (val.value_kind() == symbol_kind::ABS && val.get_abs() >= 0) ? val.get_abs() : 0;
		space::resolve(std::get<space_ptr>(target.value), length);
	}
	else if (target.kind() == dependant_kind::SYMBOL)
	{
		sym_ctx_.get_symbol(std::get<id_index>(target.value))->set_value(val);
	}
}

void symbol_dependency_tables::resolve_dependant_default(dependant target)
{
	if (target.kind() == dependant_kind::SPACE)
	{
		space::resolve(std::get<space_ptr>(target.value), 1);
	}
	else if (target.kind() == dependant_kind::SYMBOL)
	{
		sym_ctx_.get_symbol(std::get<id_index>(target.value))->set_value(0);
	}
}

void symbol_dependency_tables::try_resolve(std::queue<dependant> newly_defined)
{
	bool defined = !newly_defined.empty();
	std::vector<dependant> to_delete;

	while (defined)
	{
		defined = false;
		for (auto& [target, dep_src] : dependencies_)
		{
			if (extract_dependencies(dep_src).empty()) // target no longer dependent on anything
			{
				to_delete.push_back(target);

				resolve_dependant(target, dep_src); //resolve target

				defined = true; //push target to newly defined to
			}
		}

		for (auto del : to_delete)
		{
			dependencies_.erase(del);
			dependency_sources_.erase(del);
		}

		to_delete.clear();
	}
}

std::vector<dependant> symbol_dependency_tables::extract_dependencies(const resolvable* dependency_source)
{
	std::vector<dependant> ret;
	auto deps = dependency_source->get_dependencies(sym_ctx_);

	ret.insert(ret.end(), std::make_move_iterator(deps.undefined_symbols.begin()), std::make_move_iterator(deps.undefined_symbols.end()));

	if (!ret.empty() || !deps.unresolved_address)
		return ret;

	for (auto& [space, count] : deps.unresolved_address->spaces)
	{
		assert(count != 0);
		ret.push_back(space);
	}

	return ret;
}

std::vector<dependant> symbol_dependency_tables::extract_dependencies(const std::vector<const resolvable*>& dependency_sources)
{
	std::vector<dependant> ret;

	for (auto dep : dependency_sources)
	{
		auto tmp = extract_dependencies(dep);
		ret.insert(ret.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
	}
	return ret;
}

symbol_dependency_tables::symbol_dependency_tables(ordinary_assembly_context& sym_ctx)
	: sym_ctx_(sym_ctx) {}

bool symbol_dependency_tables::add_dependency(id_index target, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt)
{
	if (dependencies_.find(target) != dependencies_.end())
		throw std::invalid_argument("symbol dependency already present");

	auto dependencies = extract_dependencies(dependency_source);

	bool no_cycle = check_cycle(target, dependencies);

	if (!no_cycle)
	{
		add_defined(target);
		return false;
	}

	dependencies_.emplace(target, dependency_source);
	dependency_sources_.emplace(target, std::move(dependency_source_stmt));

	return true;
}

void symbol_dependency_tables::add_dependency(post_stmt_ptr target, std::vector<const resolvable*> dependencies)
{
	postponed_stmts_.emplace_back(std::move(target), std::move(dependencies));
}

void symbol_dependency_tables::add_dependency(space_ptr space, const resolvable* dependency_source, post_stmt_ptr dependency_source_stmt)
{
	auto [it,emplaced] = dependencies_.emplace(space, dependency_source);
	dependency_sources_.emplace(space, std::move(dependency_source_stmt));

	if(!emplaced)
		throw std::invalid_argument("symbol dependency already present");
}

void symbol_dependency_tables::add_defined(id_index target)
{
	std::queue<dependant> newly_defined;
	newly_defined.push(target);

	try_resolve(std::move(newly_defined));
}

bool symbol_dependency_tables::check_loctr_cycle()
{
	using dep_map = std::unordered_map<dependant, std::vector<dependant>, dependant_hash>;
	using dep_set = std::unordered_set<dependant, dependant_hash>;

	dep_map dep_g;
	dep_set cycled;
	std::stack<std::vector<dependant>> path_stack;
	std::unordered_map<dependant, dep_set, dependant_hash> visited;

	//create graph
	for (auto& [target,dep_src] : dependencies_)
	{
		if (target.kind() == dependant_kind::SPACE)
		{
			auto new_deps = extract_dependencies(dep_src);
			if (!new_deps.empty() && new_deps.front().kind() == dependant_kind::SPACE)
			{
				dep_g.emplace(target, std::move(new_deps));
			}
		}
	}

	//find cycle
	for (auto& [v, e] : dep_g)
	{
		//if graph has more than one component
		if (visited.find(v) == visited.end())
			path_stack.push({ v });
		else
			continue;

		while (!path_stack.empty())
		{
			auto path = std::move(path_stack.top());
			auto target = path.back();
			path_stack.pop();

			//if edge already visited, continue
			if (path.size() > 1)
			{
				auto& visited_edges = visited[*(path.end() - 2)];
				if (visited_edges.find(target) != visited_edges.end())
					continue;
			}

			//if cycle found, register to cycled
			if (visited.find(target) != visited.end())
			{
				auto cycle_start = std::find(path.begin(), path.end(), target);
				cycled.insert(cycle_start, path.end());
			}

			//register visited edge
			if (path.size() > 1)
				visited[*(path.end() - 2)].insert(target);
			else 
				visited[target];
			
			//add next paths
			auto it = dep_g.find(target);
			if (it != dep_g.end())
				for (auto& entry : it->second)
				{
					auto new_path(path);
					new_path.push_back(entry);
					path_stack.push(std::move(new_path));
				}
		}
	}

	for (auto target : cycled)
	{
		resolve_dependant_default(target);
		dependencies_.erase(target);
		dependency_sources_.erase(target);
	}

	return cycled.empty();
}

void symbol_dependency_tables::add_defined(const std::vector<id_index>& target)
{
	std::queue<dependant> newly_defined;
	for (auto id : target)
		newly_defined.push(id);

	try_resolve(std::move(newly_defined));
}

std::vector<post_stmt_ptr> symbol_dependency_tables::collect_postponed()
{
	std::vector<post_stmt_ptr> res;

	for (auto& [stmt, _] : postponed_stmts_)
	{
		res.push_back(std::move(stmt));
	}

	for (auto& [_, stmt] : dependency_sources_)
	{
		res.push_back(std::move(stmt));
	}

	postponed_stmts_.clear();
	dependency_sources_.clear();
	dependencies_.clear();

	return res;
}

dependant::dependant(id_index symbol)
	:value(symbol) {}


dependant::dependant(space_ptr space_id)
	: value(std::move(space_id)) {}

bool dependant::operator==(const dependant& oth) const
{
	return value == oth.value;
}

dependant_kind dependant::kind() const
{
	return static_cast<dependant_kind>(value.index());
}

std::size_t dependant_hash::operator()(const dependant& k) const
{
	return std::hash<decltype(k.value)>{}(k.value);
}
