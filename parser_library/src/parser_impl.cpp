#include "parser_impl.h"
#include "../include/shared/lexer.h"
#include "../include/shared/token_stream.h"
#include "generated/hlasmparser.h"
#include "semantics/processing_manager.h"

using namespace hlasm_plugin::parser_library;

hlasm_plugin::parser_library::parser_impl::parser_impl(antlr4::TokenStream * input) : Parser(input) {}

semantics::symbol_occurence parser_impl::create_occurence(semantics::symbol_range range, std::string name, bool make_it_candidate)
{
	return { range, name, file_name, current_macro_def.name, make_it_candidate };
}

bool parser_impl::is_var_definition(const semantics::symbol_occurence & occ)
{
	std::string var_sym_def_instr_label[] = { "SETA", "SETB", "SETC" };
	std::string var_sym_def_instr_op[] = { "LCLA", "LCLB", "LCLC", "GBLA","GBLB","GBLC" };

	return ((std::find(std::begin(var_sym_def_instr_label), std::end(var_sym_def_instr_label), collector.current_instruction().ordinary_name) != std::end(var_sym_def_instr_label) && collector.current_label().variable_symbol.name == occ.name)
		|| (std::find(std::begin(var_sym_def_instr_op), std::end(var_sym_def_instr_op), collector.current_instruction().ordinary_name) != std::end(var_sym_def_instr_op) && std::find_if(collector.current_operands_and_remarks().operands.begin(), collector.current_operands_and_remarks().operands.end(), [&occ](auto && operand) { return operand->range == occ.range;  }) != collector.current_operands_and_remarks().operands.end())
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

bool hlasm_plugin::parser_library::parser_impl::is_last_line()
{
	return dynamic_cast<lexer*>(_input->getTokenSource())->is_last_line();
}

void hlasm_plugin::parser_library::parser_impl::rewind_input(hlasm_plugin::parser_library::location location)
{
	dynamic_cast<lexer*>(_input->getTokenSource())->rewind_input(location);
}

semantics::operand_remark_semantic_info hlasm_plugin::parser_library::parser_impl::reparse_operand_remark_field(std::string field)
{
	//todo set correct ranges, propagate errors
	semantics::operand_remark_semantic_info ret;

	input_source input(std::move(field));
	hlasm_plugin::parser_library::lexer lex(&input);
	lex.set_unlimited_line(true);
	token_stream tokens(&lex);
	generated::hlasmparser operand_parser(&tokens);
	operand_parser.format = format;
	operand_parser.removeErrorListeners();
	auto res = operand_parser.operands_model();
	ret.operands = std::move(res->line.operands);
	ret.remarks = std::move(res->line.remarks);

	return ret;
}

hlasm_plugin::parser_library::location hlasm_plugin::parser_library::parser_impl::statement_start() const
{
	return dynamic_cast<lexer*>(_input->getTokenSource())->last_lln_begin_location();
}

void hlasm_plugin::parser_library::parser_impl::initialize(semantics::processing_manager* proc_mngr)
{
	mngr = proc_mngr;
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

void hlasm_plugin::parser_library::parser_impl::process_instruction()
{
	if (!collector.current_instruction().ordinary_name.empty())
	{
		if (current_macro_def.just_defined)
		{
			current_macro_def.name = collector.current_instruction().ordinary_name;
			check_definition_candidates();
		}
		else if (collector.current_instruction().ordinary_name == "MACRO")
			current_macro_def.just_defined = true;
		else if (collector.current_instruction().ordinary_name == "MEND")
			current_macro_def.name = "";
		else
			check_definition_candidates();
	}

	mngr->process_instruction(collector.extract_instruction_field());
}

void hlasm_plugin::parser_library::parser_impl::process_statement()
{
	mngr->process_statement(collector.extract_statement());
	collector.prepare_for_next_statement();
}
