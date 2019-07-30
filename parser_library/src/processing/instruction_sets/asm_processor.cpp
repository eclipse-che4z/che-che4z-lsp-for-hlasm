#include "asm_processor.h"
#include "postponed_statement_impl.h"
#include "../context_manager.h"
#include "../../checking/instr_operand.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

void asm_processor::process_sect(const context::section_kind kind,rebuilt_statement stmt)
{
	auto sect_name = find_label_symbol(stmt);

	if (hlasm_ctx.ord_ctx.symbol_defined(sect_name) && !hlasm_ctx.ord_ctx.section_defined(sect_name, kind))
	{
		add_diagnostic(diagnostic_s::error_E031("", "symbol", stmt.label_ref().field_range));
	}
	else
	{
		hlasm_ctx.ord_ctx.set_section(sect_name, kind);
	}
	check(stmt,hlasm_ctx,checker_,*this);
}

void asm_processor::process_LOCTR(rebuilt_statement stmt)
{
	auto loctr_name = find_label_symbol(stmt);

	if (loctr_name == context::id_storage::empty_id)
		add_diagnostic(diagnostic_s::error_E053("", "", stmt.label_ref().field_range));

	if (hlasm_ctx.ord_ctx.symbol_defined(loctr_name) && !hlasm_ctx.ord_ctx.counter_defined(loctr_name))
	{
		add_diagnostic(diagnostic_s::error_E031("", "symbol", stmt.label_ref().field_range));
	}
	else
	{
		hlasm_ctx.ord_ctx.set_location_counter(loctr_name);
	}
	check(stmt, hlasm_ctx, checker_, *this);
}

void asm_processor::process_EQU(rebuilt_statement stmt)
{
	auto symbol_name = find_label_symbol(stmt);

	if (symbol_name == context::id_storage::empty_id)
	{
		if(stmt.label_ref().type == semantics::label_si_type::EMPTY)
			add_diagnostic(diagnostic_s::error_E053("", "", stmt.label_ref().field_range));
		return;
	}

	if (hlasm_ctx.ord_ctx.symbol_defined(symbol_name))
	{
		add_diagnostic(diagnostic_s::error_E031("", "symbol", stmt.label_ref().field_range));
		return;
	}

	if (stmt.operands_ref().value.size() != 0 && stmt.operands_ref().value[0]->type != semantics::operand_type::UNDEF)
	{
		auto asm_op = stmt.operands_ref().value[0]->access_asm();
		auto expr_op = asm_op->access_expr();

		if (expr_op)
		{
			auto holder(expr_op->expression->get_dependencies(hlasm_ctx.ord_ctx));

			if (!holder.contains_dependencies())
			{
				hlasm_ctx.ord_ctx.create_symbol(symbol_name, expr_op->expression->resolve(hlasm_ctx.ord_ctx), context::symbol_attributes());
				hlasm_ctx.ord_ctx.symbol_dependencies.add_defined(symbol_name);
			}
			else
			{
				if (holder.is_address())
					hlasm_ctx.ord_ctx.create_symbol(symbol_name, *holder.unresolved_address, context::symbol_attributes());
				else
					hlasm_ctx.ord_ctx.create_symbol(symbol_name, context::symbol_value(), context::symbol_attributes());

				std::vector<const context::resolvable*> tmp = { &*expr_op->expression };
				add_dependency(stmt.stmt_range_ref(), symbol_name, tmp, std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()));
			}
		}
	}
}

void asm_processor::process_DC(rebuilt_statement stmt)
{
	auto label = find_label_symbol(stmt);

	if (label != context::id_storage::empty_id && !hlasm_ctx.ord_ctx.symbol_defined(label))
		hlasm_ctx.ord_ctx.create_symbol(label, context::symbol_value(0), {});

	if (!stmt.operands_ref().value.empty())
	{
		if (stmt.operands_ref().value[0]->type == semantics::operand_type::EMPTY || stmt.operands_ref().value[0]->type == semantics::operand_type::UNDEF)
			return;

		auto dat_op = stmt.operands_ref().value[0]->access_data_def();
		assert(dat_op);

		if (dat_op->value->dupl_factor && dat_op->value->dupl_factor->get_dependencies(hlasm_ctx.ord_ctx).contains_dependencies())
		{
			auto space = hlasm_ctx.ord_ctx.register_space();
			add_dependency(stmt.stmt_range_ref(), space, dat_op->value->dupl_factor.get(), std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()));
		}
	}
}

void asm_processor::process_COPY(rebuilt_statement stmt)
{
}

asm_processor::asm_processor(context::hlasm_context& hlasm_ctx, branching_provider& provider, statement_field_reparser& parser)
	:low_language_processor(hlasm_ctx, provider,parser),table_(create_table(hlasm_ctx)) {}

void asm_processor::process(context::shared_stmt_ptr stmt)
{
	process(preprocess(stmt));
}

void asm_processor::process(context::unique_stmt_ptr stmt)
{
	process(preprocess(std::move(stmt)));
}

asm_processor::process_table_t asm_processor::create_table(context::hlasm_context& ctx)
{
	process_table_t table;
	table.emplace(ctx.ids().add("CSECT"),
		std::bind(&asm_processor::process_sect, this, context::section_kind::EXECUTABLE, std::placeholders::_1));
	table.emplace(ctx.ids().add("DSECT"),
		std::bind(&asm_processor::process_sect, this, context::section_kind::DUMMY, std::placeholders::_1));
	table.emplace(ctx.ids().add("RSECT"),
		std::bind(&asm_processor::process_sect, this, context::section_kind::READONLY, std::placeholders::_1));
	table.emplace(ctx.ids().add("COM"),
		std::bind(&asm_processor::process_sect, this, context::section_kind::COMMON, std::placeholders::_1));
	table.emplace(ctx.ids().add("DXD"),
		std::bind(&asm_processor::process_sect, this, context::section_kind::EXTERNAL, std::placeholders::_1));
	table.emplace(ctx.ids().add("LOCTR"),
		std::bind(&asm_processor::process_LOCTR, this, std::placeholders::_1));
	table.emplace(ctx.ids().add("EQU"),
		std::bind(&asm_processor::process_EQU, this, std::placeholders::_1));
	table.emplace(ctx.ids().add("DC"),
		std::bind(&asm_processor::process_DC, this, std::placeholders::_1));

	return table;
}

context::id_index asm_processor::find_label_symbol(const rebuilt_statement& stmt)
{
	switch (stmt.label_ref().type)
	{
	case semantics::label_si_type::ORD:
		return context_manager(hlasm_ctx).get_symbol_name(std::get<std::string>(stmt.label_ref().value));
	case semantics::label_si_type::EMPTY:
	case semantics::label_si_type::SEQ:
		return context::id_storage::empty_id;
	default:
		return context::id_storage::empty_id;
	}
}

context::id_index asm_processor::find_sequence_symbol(const rebuilt_statement& stmt)
{
	switch (stmt.label_ref().type)
	{
	case semantics::label_si_type::ORD:
	case semantics::label_si_type::EMPTY:
		return context::id_storage::empty_id;
	case semantics::label_si_type::SEQ:
		return std::get<semantics::seq_sym>(stmt.label_ref().value).name;
	default:
		return context::id_storage::empty_id;
	}
}

void asm_processor::process(rebuilt_statement statement)
{
	auto it = table_.find(statement.opcode_ref().value);
	if (it != table_.end()) 
	{
		auto& [key, func] = *it;
		func(std::move(statement));
	}
	else
	{
		check(std::move(statement), hlasm_ctx, checker_, *this);
	}
}
