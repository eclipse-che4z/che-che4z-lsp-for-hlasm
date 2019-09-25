#include "mach_processor.h"
#include "postponed_statement_impl.h"
#include "../context_manager.h"
#include "../../context/instruction.h"
#include "../../parser_impl.h"

#include <algorithm>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

mach_processor::mach_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider, statement_fields_parser& parser)
	:low_language_processor(hlasm_ctx, provider,parser) {}

void mach_processor::process(context::shared_stmt_ptr stmt)
{
	process(preprocess(stmt), stmt->access_resolved()->opcode_ref());
}

void mach_processor::process(context::unique_stmt_ptr stmt)
{
	auto opcode = stmt->access_resolved()->opcode_ref();
	process(preprocess(std::move(stmt)), opcode);
}

void mach_processor::process(rebuilt_statement stmt, const op_code& opcode)
{
	auto mnem_tmp = context::instruction::mnemonic_codes.find(*opcode.value);

	auto tmp = mnem_tmp != context::instruction::mnemonic_codes.end() ?
		context::instruction::machine_instructions.find(mnem_tmp->second.instruction) :
		context::instruction::machine_instructions.find(*opcode.value);

	assert(tmp != context::instruction::machine_instructions.end());

	auto& [name, instr] = *tmp;

	if (stmt.label_ref().type == semantics::label_si_type::ORD)
	{
		auto label_name = context_manager(hlasm_ctx).get_symbol_name(std::get<std::string>(stmt.label_ref().value));
		if (hlasm_ctx.ord_ctx.symbol_defined(label_name))
		{
			add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
		}
		else
		{
			auto addr = hlasm_ctx.ord_ctx.align(context::no_align);

			create_symbol(stmt.stmt_range_ref(), label_name, addr,
				context::symbol_attributes::make_machine_attrs((context::symbol_attributes::len_attr)instr->size_for_alloc / 8));
		}
	}

	std::vector<const context::resolvable*> dependencies;
	for (auto& op : stmt.operands_ref().value)
	{
		auto evaluable = dynamic_cast<semantics::evaluable_operand*>(op.get());
		if (evaluable)
		{
			if (evaluable->has_dependencies(hlasm_ctx.ord_ctx))
			{
				auto deps_tmp = evaluable->get_resolvables();
				dependencies.insert(dependencies.end(), std::make_move_iterator(deps_tmp.begin()), std::make_move_iterator(deps_tmp.end()));
			}
		}
	}

	if (!dependencies.empty())
		hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()), dependencies);
	else
		check(stmt, hlasm_ctx, checker, *this);

	hlasm_ctx.ord_ctx.reserve_storage_area(instr->size_for_alloc / 8, context::no_align);
}
