#include "asm_processor.h"
#include "postponed_statement_impl.h"
#include "../context_manager.h"
#include "../../expressions/mach_expr_term.h"
#include "../../checking/instr_operand.h"
#include "data_def_postponed_statement.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

void asm_processor::process_sect(const context::section_kind kind,rebuilt_statement stmt)
{
	auto sect_name = find_label_symbol(stmt);

	if (hlasm_ctx.ord_ctx.symbol_defined(sect_name) && !hlasm_ctx.ord_ctx.section_defined(sect_name, kind))
	{
		add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
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
		add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));

	if (hlasm_ctx.ord_ctx.symbol_defined(loctr_name) && !hlasm_ctx.ord_ctx.counter_defined(loctr_name))
	{
		add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
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
			add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));
		return;
	}

	if (hlasm_ctx.ord_ctx.symbol_defined(symbol_name))
	{
		add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
		return;
	}
	
	std::optional<context::symbol_attributes::len_attr> length_attr;

	if (stmt.operands_ref().value.size() >= 2 && stmt.operands_ref().value[1]->type != semantics::operand_type::EMPTY) //length attribute operand
	{
		auto asm_op = stmt.operands_ref().value[1]->access_asm();
		auto expr_op = asm_op->access_expr();

		if (expr_op && !expr_op->has_dependencies(hlasm_ctx.ord_ctx))
		{
			auto length_value = expr_op->expression->resolve(hlasm_ctx.ord_ctx);
			if (length_value.value_kind() == context::symbol_kind::ABS && length_value.get_abs() >= 0 && length_value.get_abs() <= 65535)
				length_attr = (context::symbol_attributes::len_attr)length_value.get_abs();
			else
				add_diagnostic(diagnostic_op::error_A133_EQU_len_att_format(asm_op->operand_range));
		}
		else
			add_diagnostic(diagnostic_op::error_A133_EQU_len_att_format(asm_op->operand_range));
	}

	if (stmt.operands_ref().value.size() != 0 && stmt.operands_ref().value[0]->type != semantics::operand_type::EMPTY)
	{
		auto asm_op = stmt.operands_ref().value[0]->access_asm();
		auto expr_op = asm_op->access_expr();

		if (expr_op)
		{
			auto holder(expr_op->expression->get_dependencies(hlasm_ctx.ord_ctx));

			context::symbol_attributes attrs;
			if (length_attr)
				attrs.length(*length_attr);

			if (!holder.contains_dependencies())
			{
				create_symbol(stmt.stmt_range_ref(), symbol_name, expr_op->expression->resolve(hlasm_ctx.ord_ctx), attrs);
			}
			else
			{
				if (!holder.is_address())
				{
					create_symbol(stmt.stmt_range_ref(), symbol_name, context::symbol_value(), attrs);
					add_dependency(stmt.stmt_range_ref(), symbol_name, &*expr_op->expression,
						std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()));
				}
				else
					create_symbol(stmt.stmt_range_ref(), symbol_name, *holder.unresolved_address, attrs);

			}
		}
	}
}

template<checking::data_instr_type instr_type>
void asm_processor::process_data_instruction(rebuilt_statement stmt)
{
	//enforce alignment of the first operand
	context::alignment al = context::no_align;
	if (!stmt.operands_ref().value.empty() && stmt.operands_ref().value[0]->type != semantics::operand_type::EMPTY)
		al = stmt.operands_ref().value[0]->access_data_def()->value->get_alignment();

	context::address adr = hlasm_ctx.ord_ctx.align(al);
	
	//dependency sources is list of all expressions in data def operand, that have some unresolved dependencies.
	std::vector<const context::resolvable*> dependency_sources;
	//has_length_dependencies specifies whether the length of the data instruction can be resolved right now or must be postponed
	bool has_length_dependencies = false;
	for (const auto& op : stmt.operands_ref().value)
	{
		if (op->type == semantics::operand_type::EMPTY)
			continue;
		auto data_op = op->access_data_def();

		data_op->value->assign_location_counter(adr);

		//dependencies

		if (data_op->value->type != 'V' && data_op->has_dependencies(hlasm_ctx.ord_ctx))
		{
			auto res = data_op->get_resolvables();
			dependency_sources.insert(dependency_sources.end(), res.begin(), res.end());
		}
		
		has_length_dependencies |= data_op->get_length_dependencies(hlasm_ctx.ord_ctx).contains_dependencies();

		//some types require operands that consist only of one symbol
		data_op->value->check_single_symbol_ok(diagnostic_collector(this));
	}

	//process label
	auto label = find_label_symbol(stmt);

	if (label != context::id_storage::empty_id)
	{
		if (!hlasm_ctx.ord_ctx.symbol_defined(label))
			create_symbol(stmt.stmt_range_ref(), label, std::move(adr), {});
		else
			add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
	}

	//TODO issue warning when alignment is bigger than section's alignment
	//hlasm_ctx.ord_ctx.current_section()->current_location_counter().


	if (!dependency_sources.empty())
	{
		auto post_stmt = std::make_unique<data_def_postponed_statement<instr_type>>(std::move(stmt), hlasm_ctx.processing_stack());

		if (has_length_dependencies)
		{
			auto sp = hlasm_ctx.ord_ctx.register_space();
			hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(sp, &*post_stmt, nullptr);
			//dependencies of whole post_stmt (dependencies of dependency_sources) is superset of dependencies of post_stmt
			//so it is safe to use raw ptr; the post_stmt will surely not be destroyed before the space is resolved.
		}
		else
			hlasm_ctx.ord_ctx.reserve_storage_area(
				data_def_postponed_statement<instr_type>::get_operands_length(post_stmt->operands_ref().value, hlasm_ctx.ord_ctx), context::no_align);

		hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::move(post_stmt), std::move(dependency_sources));
	}
	else
	{
		hlasm_ctx.ord_ctx.reserve_storage_area(
			data_def_postponed_statement<instr_type>::get_operands_length(stmt.operands_ref().value, hlasm_ctx.ord_ctx), context::no_align);
		check(stmt, hlasm_ctx, checker_, *this);
	}
}

void asm_processor::process_DC(rebuilt_statement stmt)
{	
	process_data_instruction<checking::data_instr_type::DC>(std::move(stmt));
}

void asm_processor::process_DS(rebuilt_statement stmt)
{
	process_data_instruction<checking::data_instr_type::DS>(std::move(stmt));
}

void asm_processor::process_COPY(rebuilt_statement stmt)
{
	find_sequence_symbol(stmt);

	if (stmt.operands_ref().value.size() == 1 && stmt.operands_ref().value.front()->access_asm()->access_expr())
	{
		process_copy(stmt, hlasm_ctx, lib_provider_, this);
	}
	else
	{
		check(stmt, hlasm_ctx, checker_, *this);
	}
}

asm_processor::asm_processor(context::hlasm_context& hlasm_ctx, branching_provider& branch_provider, parse_lib_provider& lib_provider, statement_fields_parser& parser)
	:low_language_processor(hlasm_ctx, branch_provider, parser), table_(create_table(hlasm_ctx)), lib_provider_(lib_provider) {}

void asm_processor::process(context::shared_stmt_ptr stmt)
{
	process(preprocess(stmt));
}

void asm_processor::process_copy(const semantics::complete_statement& stmt, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, diagnosable_ctx* diagnoser)
{
	auto& expr = stmt.operands_ref().value.front()->access_asm()->access_expr()->expression;
	auto sym_expr = dynamic_cast<expressions::mach_expr_symbol*>(expr.get());

	if (!sym_expr)
	{
		if(diagnoser)
			diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().value.front()->operand_range));
		return;
	}

	auto tmp = hlasm_ctx.copy_members().find(sym_expr->value);

	if (tmp == hlasm_ctx.copy_members().end())
	{
		bool result = lib_provider.parse_library(*sym_expr->value, hlasm_ctx, library_data{ context::file_processing_type::COPY, sym_expr->value });

		if (!result)
		{
			if(diagnoser)
				diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().value.front()->operand_range));
			return;
		}
	}

	auto cycle_tmp = std::find_if(hlasm_ctx.copy_stack().begin(), hlasm_ctx.copy_stack().end(), [&](auto& entry) {return entry.name == sym_expr->value; });

	if (cycle_tmp != hlasm_ctx.copy_stack().end())
	{
		if (diagnoser)
			diagnoser->add_diagnostic(diagnostic_op::error_E062(stmt.stmt_range_ref()));
		return;
	}

	hlasm_ctx.enter_copy_member(sym_expr->value);
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
	table.emplace(ctx.ids().add("DS"),
		std::bind(&asm_processor::process_DS, this, std::placeholders::_1));
	table.emplace(ctx.ids().add("COPY"),
		std::bind(&asm_processor::process_COPY, this, std::placeholders::_1));

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
	semantics::seq_sym symbol;
	switch (stmt.label_ref().type)
	{
	case semantics::label_si_type::SEQ:
		symbol = std::get<semantics::seq_sym>(stmt.label_ref().value);
		provider.register_sequence_symbol(symbol.name,symbol.symbol_range);
		return symbol.name;
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
		//until implementation of all instructions, if has deps, ignore 
		for (auto& op : statement.operands_ref().value)
		{
			auto tmp = context::instruction::assembler_instructions.find(*statement.opcode_ref().value);
			bool can_have_ord_syms = tmp != context::instruction::assembler_instructions.end() ? tmp->second.has_ord_symbols : true;

			if (op->type != semantics::operand_type::EMPTY && can_have_ord_syms  && op->access_asm()->has_dependencies(hlasm_ctx.ord_ctx))
				return;
		}
		check(statement, hlasm_ctx, checker_, *this);
	}
}
