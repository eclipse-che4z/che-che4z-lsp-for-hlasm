#include "processing_manager.h"
#include "statement_processors/macrodef_processor.h"
#include "statement_processors/lookahead_processor.h"
#include "statement_processors/ordinary_processor.h"
#include "statement_processors/copy_processor.h"
#include "statement_processors/empty_processor.h"
#include "opencode_provider.h"
#include "macro_statement_provider.h"
#include "copy_statement_provider.h"
#include "../parser_impl.h"

#include <assert.h>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

processing_manager::processing_manager(
	provider_ptr base_provider, 
	context::hlasm_context& hlasm_ctx,
	const library_data data,
	parse_lib_provider& lib_provider, 
	statement_fields_parser& parser)
	: diagnosable_ctx(hlasm_ctx), hlasm_ctx_(hlasm_ctx), lib_provider_(lib_provider)
{
	provs_.emplace_back(std::make_unique<macro_statement_provider>(hlasm_ctx,parser));
	provs_.emplace_back(std::make_unique<copy_statement_provider>(hlasm_ctx,parser));
	provs_.emplace_back(std::move(base_provider));

	switch (data.proc_type)
	{
	case context::file_processing_type::OPENCODE:
		procs_.emplace_back(std::make_unique<ordinary_processor>(hlasm_ctx, lib_provider, *this, *this, parser));
		break;
	case context::file_processing_type::COPY:
		procs_.emplace_back(std::make_unique<copy_processor>(hlasm_ctx, *this, copy_start_data{ data.library_member }));
		break;
	case context::file_processing_type::MACRO:
		procs_.emplace_back(std::make_unique<macrodef_processor>(hlasm_ctx, *this,lib_provider, macrodef_start_data(data.library_member)));
		break;
	default:
		break;
	}
}

void processing_manager::start_processing()
{
	while (!procs_.empty())
	{
		statement_processor& proc = *procs_.back();
		statement_provider& prov = find_provider();

		if ((prov.finished() && proc.terminal_condition(prov.kind)) ||
			proc.finished())
		{
			finish_processor();
			continue;
		}

		prov.process_next(proc);
	}
}

statement_provider& processing_manager::find_provider()
{
	for (auto& prov : provs_) {
		if (!prov->finished())
			return *prov;
	}

	return *provs_.back();
}

void processing_manager::finish_processor()
{
	procs_.back()->end_processing();
	collect_diags_from_child(*procs_.back());
	procs_.pop_back();
}

void processing_manager::start_macro_definition(const macrodef_start_data start)
{
	procs_.emplace_back(std::make_unique<macrodef_processor>(hlasm_ctx_, *this, lib_provider_, start));
}

void processing_manager::finish_macro_definition(macrodef_processing_result result)
{
	hlasm_ctx_.add_macro(
		result.prototype.macro_name,
		result.prototype.name_param,
		std::move(result.prototype.symbolic_params),
		std::move(result.definition),
		std::move(result.nests),
		std::move(result.sequence_symbols),
		std::move(result.definition_location));
}

void processing_manager::start_lookahead(const lookahead_start_data start)
{
	procs_.emplace_back(std::make_unique < lookahead_processor>(hlasm_ctx_, *this, *this,lib_provider_, std::move(start)));
}

void processing_manager::finish_lookahead(lookahead_processing_result result)
{
	if (result.success)
		jump_in_statements(result.target,result.target_range);
	else
		dynamic_cast<opencode_provider*>(provs_.back().get())->rewind_input(result.source);

	find_provider();

	if (!result.success) //skip next statement
	{
		empty_processor tmp(hlasm_ctx_); 
		provs_.back()->process_next(tmp);
	}
}

void processing_manager::start_copy_member(const copy_start_data start)
{
	procs_.emplace_back(std::make_unique < copy_processor>(hlasm_ctx_, *this, std::move(start)));
}

void processing_manager::finish_copy_member(copy_processing_result result)
{
	hlasm_ctx_.add_copy_member(
		result.member_name, 
		result.invalid_member ? context::statement_block() : std::move(result.definition),
		std::move(result.definition_location));
}

void processing_manager::jump_in_statements(context::id_index target, range symbol_range)
{
	auto symbol = hlasm_ctx_.get_sequence_symbol(target);
	if (!symbol)
	{
		if (hlasm_ctx_.is_in_macro())
		{
			add_diagnostic(diagnostic_op::error_E047(*target, symbol_range));
		}
		else
			start_lookahead(lookahead_start_data{
				lookahead_action::SEQ,
				target,
				symbol_range,
				dynamic_cast<opencode_provider*>(provs_.back().get())->statement_start()
			});
	}
	else
	{
		if (symbol->kind == context::sequence_symbol_kind::MACRO)
		{
			assert(hlasm_ctx_.is_in_macro());
			hlasm_ctx_.scope_stack().back().this_macro->current_statement = (int)symbol->access_macro_symbol()->statement_offset - 1;
		}
		else
		{
			auto opencode_symbol = symbol->access_opencode_symbol();

			auto loc = opencode_symbol->statement_position;
			dynamic_cast<opencode_provider*>(provs_.back().get())->rewind_input(loc);

			hlasm_ctx_.apply_copy_frame_stack(opencode_symbol->copy_stack);
		}

		hlasm_ctx_.decrement_branch_counter();
	}
}

void processing_manager::register_sequence_symbol(context::id_index target, range symbol_range)
{
	if (hlasm_ctx_.is_in_macro())
		return;

	auto symbol = hlasm_ctx_.get_sequence_symbol(target);
	auto new_symbol = create_opencode_sequence_symbol(target, symbol_range);

	if (!symbol)
	{
		hlasm_ctx_.add_sequence_symbol(std::move(new_symbol));
	}
	else if(!(*symbol->access_opencode_symbol() == *new_symbol->access_opencode_symbol()))
	{
		add_diagnostic(diagnostic_op::error_E045(*target, symbol_range));
	}
}

context::sequence_symbol_ptr processing_manager::create_opencode_sequence_symbol(context::id_index name, range symbol_range)
{
	auto opencode_prov = dynamic_cast<opencode_provider*>(provs_.back().get());
	auto symbol_pos = symbol_range.start;
	location loc(symbol_pos, hlasm_ctx_.processing_stack().back().file);

	std::vector<context::opencode_sequence_symbol::copy_frame> frame_stack;

	for (size_t i = 0; i < hlasm_ctx_.copy_stack().size(); ++i)
	{
		auto& invo = hlasm_ctx_.copy_stack()[i];
		frame_stack.push_back({
			invo.name,
			(size_t)invo.current_statement - (i == hlasm_ctx_.copy_stack().size() - 1 ? 1 : 0)
			});
	}

	context::opencode_sequence_symbol::opencode_position opencode_pos;

	if (frame_stack.empty())
		opencode_pos = opencode_prov->statement_start();
	else
	{
		opencode_pos = opencode_prov->statement_end();
		++opencode_pos.file_line;
	}

	return std::make_unique<context::opencode_sequence_symbol>(name, loc, opencode_pos, std::move(frame_stack));
}

void processing_manager::collect_diags() const
{
	for (auto& proc : procs_)
		collect_diags_from_child(*proc);

	collect_diags_from_child(dynamic_cast<parser_impl&>(*provs_.back()));
}
