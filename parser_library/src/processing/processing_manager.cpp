#include "processing_manager.h"
#include "statement_processors/macrodef_processor.h"
#include "statement_processors/lookahead_processor.h"
#include "statement_processors/ordinary_processor.h"
#include "opencode_provider.h"
#include "statement_processors/empty_processor.h"
#include "macro_statement_provider.h"

#include <assert.h>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

processing_manager::processing_manager(provider_ptr base_provider, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, statement_field_reparser& parser)
	: diagnosable_ctx(hlasm_ctx), hlasm_ctx_(hlasm_ctx), lib_provider_(lib_provider)
{
	provs_.emplace_back(std::make_unique<macro_statement_provider>(hlasm_ctx,parser));
	provs_.emplace_back(std::move(base_provider));

	procs_.emplace_back(std::make_unique<ordinary_processor>(hlasm_ctx, lib_provider, *this, *this, parser));
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
	if (hlasm_ctx_.current_file_proc_type() != context::file_processing_type::OPENCODE)
		return *provs_.back();

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
	procs_.emplace_back(std::make_unique<macrodef_processor>(hlasm_ctx_, *this, start));
}

void processing_manager::finish_macro_definition(macrodef_processing_result result)
{
	hlasm_ctx_.add_macro(
		result.prototype.macro_name,
		result.prototype.name_param,
		std::move(result.prototype.symbolic_params),
		std::move(result.definition),
		std::move(result.sequence_symbols),
		std::move(result.definition_location));
}

void processing_manager::start_lookahead(const lookahead_start_data start)
{
	procs_.emplace_back(std::make_unique < lookahead_processor>(hlasm_ctx_, *this, *this, std::move(start)));
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

void processing_manager::jump_in_statements(context::id_index target, range symbol_range)
{
	auto symbol = hlasm_ctx_.get_sequence_symbol(target);
	if (!symbol)
	{
		if (hlasm_ctx_.is_in_macro())
		{
			add_diagnostic(diagnostic_s::error_E047("", *target, symbol_range));
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
		if (hlasm_ctx_.get_branch_counter() < 0)
		{
			add_diagnostic(diagnostic_s::error_E056("", *target, symbol_range));
			return;
		}

		if (symbol->kind == context::sequence_symbol_kind::MACRO)
		{
			assert(hlasm_ctx_.is_in_macro());
			hlasm_ctx_.scope_stack().back().this_macro->current_statement = symbol->access_macro_symbol()->statement_offset - 1;
		}
		else
		{
			auto loc = symbol->access_opencode_symbol()->statement_position;
			dynamic_cast<opencode_provider*>(provs_.back().get())->rewind_input(loc);
		}

		hlasm_ctx_.decrement_branch_counter();
	}
}

void processing_manager::register_sequence_symbol(context::id_index target, range symbol_range)
{
	if (hlasm_ctx_.is_in_macro())
		return;

	auto symbol = hlasm_ctx_.get_sequence_symbol(target);
	auto opencode_pos = dynamic_cast<opencode_provider*>(provs_.back().get())->statement_start();

	if (!symbol)
	{
		auto symbol_pos = symbol_range.start;
		location loc(symbol_pos, hlasm_ctx_.opencode_file_name());
		hlasm_ctx_.add_sequence_symbol(std::make_unique<context::opencode_sequence_symbol>(target, loc, opencode_pos));
	}
	else if (!(symbol->access_opencode_symbol()->statement_position == opencode_pos))
	{
		add_diagnostic(diagnostic_s::error_E045("", *target, symbol_range));
	}
}

void processing_manager::collect_diags() const
{
	for (auto& proc : procs_)
		collect_diags_from_child(*proc);
}
