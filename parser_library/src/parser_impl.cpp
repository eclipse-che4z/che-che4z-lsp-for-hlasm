#include "parser_impl.h"
#include "../include/shared/token_stream.h"

using namespace hlasm_plugin::parser_library;

semantics::symbol_occurence parser_impl::create_occurence(semantics::symbol_range range, std::string name, bool make_it_candidate)
{
	return { range, name, file_name, current_macro_def.name, make_it_candidate };
}

bool parser_impl::is_var_definition(const semantics::symbol_occurence & occ)
{
	std::string var_sym_def_instr_label[] = { "SETA", "SETB", "SETC" };
	std::string var_sym_def_instr_op[] = { "LCLA", "LCLB", "LCLC", "GBLA","GBLB","GBLC" };

	return ((std::find(std::begin(var_sym_def_instr_label), std::end(var_sym_def_instr_label), *analyzer.current_instruction().id) != std::end(var_sym_def_instr_label) && analyzer.current_label().variable_symbol.name == occ.name)
		|| (std::find(std::begin(var_sym_def_instr_op), std::end(var_sym_def_instr_op), *analyzer.current_instruction().id) != std::end(var_sym_def_instr_op) && std::find_if(analyzer.current_operands_and_remarks().operands.begin(), analyzer.current_operands_and_remarks().operands.end(), [&occ](auto && operand) { return operand->range == occ.range;  }) != analyzer.current_operands_and_remarks().operands.end())
		|| current_macro_def.just_defined);
}

bool parser_impl::is_seq_definition(const semantics::symbol_occurence & occ)
{
	return occ.range.begin_col == 0;
}

void parser_impl::check_definition_candidates()
{
	semantics::symbol_occurence * current;
	for (size_t i = 0; i <  semantic_info.var_symbols.occurences.size(); ++i)
	{
		current = &semantic_info.var_symbols.occurences[i];
		if (current->candidate && is_var_definition(*current))
		{
			current->scope = current_macro_def.name;
			current->candidate = false;
			semantic_info.var_symbols.definitions.push_back(*current);
			semantic_info.var_symbols.occurences.erase(semantic_info.var_symbols.occurences.begin() + i);
			--i;
		}
		else
			current->candidate = false;
	}
	current_macro_def.just_defined = false;
}

void parser_impl::enable_continuation()
{
	dynamic_cast<token_stream*>(_input)->enable_continuation();
}

void parser_impl::disable_continuation()
{
	dynamic_cast<token_stream*>(_input)->disable_continuation();
}

bool parser_impl::is_self_def()
{
	std::string tmp(_input->LT(1)->getText());
	hlasm_plugin::parser_library::context::to_upper(tmp);
	return tmp == "B" || tmp == "X" || tmp == "C" || tmp == "G";
}


void parser_impl::initialize(std::shared_ptr<context::hlasm_context> ctx, lexer* lexer)
{
	analyzer.initialize(file_name, std::move(ctx), lexer);
}

void parser_impl::initialize(const semantics::semantic_analyzer& analyzer_init)
{
	this->analyzer.initialize(analyzer_init);
}


void parser_impl::add_diag(diagnostic_op diag)
{
	//range from diag
	add_diagnostic(diagnostic_s(file_name, {}, diag.severity, diag.code, "HLASM plugin", std::move(diag.message), {}));
}

void parser_impl::collect_diags() const
{
	collect_diags_from_child(analyzer);
}
