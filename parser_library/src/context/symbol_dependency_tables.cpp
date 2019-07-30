#include "symbol_dependency_tables.h"
#include "ordinary_assembly_context.h"

#include <stdexcept>
#include <algorithm>
#include <queue>
#include <stack>

using namespace hlasm_plugin::parser_library::context;

bool symbol_dependency_tables::check_cycle(id_index target,const std::vector<id_index>& dependencies)
{
	if (std::find(dependencies.begin(), dependencies.end(), target) != dependencies.end()) //dependencies contain target itself
		return false;

	std::stack<id_index> deps_stack;
	for (auto tmp : dependencies)
		deps_stack.push(tmp);

	while (!deps_stack.empty())
	{
		auto top_dep = deps_stack.top();
		deps_stack.pop();

		auto it = dependencies_.find(top_dep);
		
		if (it != dependencies_.end())
		{
			auto& [_, deps] = *it;
			for (auto dep : deps)
			{
				if (dep == target) return false;
				deps_stack.push(dep);
			}
		}
	}

	return true;
}

std::vector<id_index> extract_dependencies(const std::vector<const resolvable*>& dependent, ordinary_assembly_context& ctx)
{
	std::vector<id_index> dependencies;
	if (dependent.size() > 1)
	{
		for (auto dep : dependent)
		{
			auto undef = dep->get_dependencies(ctx).undefined_symbols;
			dependencies.insert(dependencies.end(), std::make_move_iterator(undef.begin()), std::make_move_iterator(undef.end()));
		}
	}
	else if (dependent.size() == 1)
	{
		auto dep_holder = dependent[0]->get_dependencies(ctx);
		if (!dep_holder.undefined_symbols.empty())
		{
			dependencies.assign(dep_holder.undefined_symbols.begin(), dep_holder.undefined_symbols.end());
		}
		else if (dep_holder.unresolved_address)
		{
			for (auto& [space, count] : dep_holder.unresolved_address->spaces)
			{
				assert(count != 0);
				dependencies.push_back(space->name);
			}
		}
	}

	return dependencies;
}

void symbol_dependency_tables::try_resolve(std::queue<id_index> newly_defined)
{
	std::vector<id_index> to_delete;

	while (!newly_defined.empty())
	{
		for (auto& [sym, deps] : dependencies_)
		{
			if (std::find(deps.begin(), deps.end(), newly_defined.front()) != deps.end())
			{
				auto& [dependable, stmt] = dependency_sources_.at(sym);

				auto new_deps = extract_dependencies(dependable, sym_ctx_);

				//new deps should be tested for cycle

				if (new_deps.empty())
				{
					to_delete.push_back(sym);

					if (id_generator::is_generated(sym))
					{
						ready_to_check_.push_back(std::move(stmt));
						generator_.release_id(sym);
						continue;
					}

					auto space_name = sym;
					auto space_it = std::find_if(spaces_.begin(), spaces_.end(), [&](auto& sp) {return sp->name == space_name; });
					if (space_it != spaces_.end())
					{
						auto length = dependable[0]->resolve(sym_ctx_).get_abs();
						assert(length >= 0);
						space::resolve(*space_it, (size_t)length);
					}
					else
					{
						auto value = dependable[0]->resolve(sym_ctx_);
						if (value.value_kind() == symbol_kind::ABS)
						{
							sym_ctx_.get_symbol(sym)->set_value(value);
						}
					}

					newly_defined.push(sym);
				}
				else
					deps = new_deps;
			}
		}
		for (auto del : to_delete)
		{
			dependencies_.erase(del);
			dependency_sources_.erase(del);
		}
		to_delete.clear();
		newly_defined.pop();
	}
}

symbol_dependency_tables::symbol_dependency_tables(ordinary_assembly_context& sym_ctx)
	: sym_ctx_(sym_ctx), generator_(sym_ctx.ids) {}

bool symbol_dependency_tables::add_dependency(id_index target, const std::vector<const resolvable*> dependent, post_stmt_ptr stmt)
{
	if (target == id_storage::empty_id)
		target = generator_.get_id();

	auto [iter,inserted] = dependency_sources_.try_emplace(target, std::move(dependent), std::move(stmt));

	if (!inserted)
		throw std::invalid_argument("symbol dependency already present");

	auto& [tmp_key, tmp_pair] = *iter;
	auto& [dep, tmp_stmt] = tmp_pair;

	auto dependencies = extract_dependencies(dep, sym_ctx_);

	bool no_cycle = check_cycle(target, dependencies);

	if (!no_cycle)
	{
		dependency_sources_.erase(iter);
		return false;
	}

	dependencies_.emplace(target, std::move(dependencies));

	return true;
}

bool hlasm_plugin::parser_library::context::symbol_dependency_tables::add_dependency(id_index target, address dependent, post_stmt_ptr stmt)
{
	resolvers_.push_back(std::make_unique<address_resolver>(std::move(dependent)));
	return add_dependency(target, { resolvers_.back().get() }, std::move(stmt));
}

bool hlasm_plugin::parser_library::context::symbol_dependency_tables::add_dependency(space_ptr space, const resolvable* dependent, post_stmt_ptr stmt)
{
	auto target = space->name;
	spaces_.push_back(std::move(space));
	return add_dependency(target, { dependent }, std::move(stmt));
}

void symbol_dependency_tables::add_defined(id_index target)
{
	std::queue<id_index> newly_defined;
	newly_defined.push(target);

	try_resolve(std::move(newly_defined));
}

void symbol_dependency_tables::add_defined(const std::vector<id_index>& target)
{
	std::queue<id_index> newly_defined;
	for (auto id : target)
		newly_defined.push(id);

	try_resolve(std::move(newly_defined));
}

std::vector<post_stmt_ptr> symbol_dependency_tables::collect_unchecked()
{
	std::vector<post_stmt_ptr> res;
	for (auto& stmt : ready_to_check_)
	{
		if (stmt)
			res.push_back(std::move(stmt));
	}
	ready_to_check_.clear();
	return res;
}

std::vector<post_stmt_ptr> symbol_dependency_tables::collect_all()
{
	std::vector<post_stmt_ptr> res(collect_unchecked());

	for (auto& [_, source] : dependency_sources_)
	{
		auto& [dependable, stmt] = source;
		if(stmt)
			res.push_back(std::move(stmt));
	}

	dependencies_.clear();
	dependency_sources_.clear();

	return res;
}
