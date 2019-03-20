#include "collector.h"
#include "../include/shared/lexer.h"

using namespace hlasm_plugin::parser_library::semantics;

hlasm_plugin::parser_library::semantics::collector::collector() : instruction_extracted_(false), statement_extracted_(false)
{
}

const label_semantic_info& hlasm_plugin::parser_library::semantics::collector::current_label()
{
	if (statement_extracted_)
		throw std::runtime_error("bad operation");
	return stmt_.label_info;
}

const instruction_semantic_info& hlasm_plugin::parser_library::semantics::collector::current_instruction()
{
	if (instruction_extracted_)
		throw std::runtime_error("bad operation");
	return stmt_.instr_info;
}

const operand_remark_semantic_info& hlasm_plugin::parser_library::semantics::collector::current_operands_and_remarks()
{
	if (statement_extracted_)
		throw std::runtime_error("bad operation");
	return stmt_.op_rem_info;
}

void hlasm_plugin::parser_library::semantics::collector::set_label_field(symbol_range range)
{
	stmt_.label_info.type = label_type::EMPTY;
	stmt_.label_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_label_field(std::string label, symbol_range range)
{
	stmt_.label_info.type = label_type::MAC;
	stmt_.label_info.name = std::move(label);
	stmt_.label_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_label_field(seq_sym sequence_symbol, symbol_range range)
{
	stmt_.label_info.type = label_type::SEQ;
	stmt_.label_info.sequence_symbol = std::move(sequence_symbol);
	stmt_.label_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_label_field(std::string label, antlr4::ParserRuleContext * parser_ctx)
{
	auto range = symbol_range::get_range(parser_ctx);

	//recognise, whether label consists only of ORDSYMBOL token
	if (parser_ctx->getStart() == parser_ctx->getStop() && parser_ctx->getStart()->getType() == lexer::Tokens::ORDSYMBOL)
	{
		stmt_.label_info.type = label_type::ORD;
		stmt_.label_info.name = std::move(label);
		stmt_.label_info.range = std::move(range);
	}
	//recognise, whether label consists of DOT ORDSYMBOL tokens, so it is sequence symbol
	else if (parser_ctx->children.size() == 2 && parser_ctx->getStart()->getType() == lexer::Tokens::DOT && parser_ctx->getStop()->getType() == lexer::Tokens::ORDSYMBOL)
	{
		stmt_.label_info.type = label_type::SEQ;
		stmt_.label_info.sequence_symbol = { std::move(label.erase(0, 1)), { parser_ctx->getStart()->getLine(), parser_ctx->getStart()->getStartIndex() } };
		stmt_.label_info.sequence_symbol.range = range;
		stmt_.label_info.range = std::move(range);
	}
	//otherwise it is macro label parameter
	else
		set_label_field(std::move(label), std::move(range));
}

void hlasm_plugin::parser_library::semantics::collector::set_label_field(concat_chain label, symbol_range range)
{
	clear_concat_chain(label);
	if (label.size() == 1 && label[0]->get_type() == concat_type::VAR) //label is variable symbol
	{
		stmt_.label_info.type = label_type::VAR;
		stmt_.label_info.variable_symbol = std::move(*label[0]->access_var());
	}
	else //label is concatenation
	{
		stmt_.label_info.type = label_type::CONC;
		stmt_.label_info.concatenation = std::move(label);
	}
	stmt_.label_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_instruction_field(symbol_range range)
{
	stmt_.instr_info.type = instr_semantic_type::EMPTY;
	stmt_.instr_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_instruction_field(std::string instr, symbol_range range)
{

	stmt_.instr_info.type = instr_semantic_type::ORD;
	stmt_.instr_info.ordinary_name = std::move(instr);
	stmt_.instr_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_instruction_field(concat_chain instr, symbol_range range)
{
	stmt_.instr_info.type = instr_semantic_type::CONC;
	stmt_.instr_info.model_name = std::move(instr);
	stmt_.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_operand_remark_field(symbol_range range)
{
	stmt_.op_rem_info.range = std::move(range);
}

void hlasm_plugin::parser_library::semantics::collector::set_operand_remark_field(concat_chain chain, symbol_range range)
{
	stmt_.op_rem_info.defered_field = std::move(chain);
	stmt_.op_rem_info.range = std::move(range);
	stmt_.op_rem_info.is_defered = true;
}

void hlasm_plugin::parser_library::semantics::collector::set_operand_remark_field(std::vector<operand_ptr> operands, std::vector<symbol_range> remarks)
{
	stmt_.op_rem_info.operands = std::move(operands);
	stmt_.op_rem_info.remarks = std::move(remarks);
}

void hlasm_plugin::parser_library::semantics::collector::set_statement_range(symbol_range range)
{
	stmt_.range = range;
}

instruction_semantic_info&& hlasm_plugin::parser_library::semantics::collector::extract_instruction_field()
{
	if (instruction_extracted_)
		throw std::runtime_error("bad operation");

	instruction_extracted_ = true;
	return std::move(stmt_.instr_info);
}

statement&& hlasm_plugin::parser_library::semantics::collector::extract_statement()
{
	if (statement_extracted_)
		throw std::runtime_error("bad operation");

	statement_extracted_ = true;
	return std::move(stmt_);
}

void hlasm_plugin::parser_library::semantics::collector::prepare_for_next_statement()
{
	instruction_extracted_ = false;
	statement_extracted_ = false;
	stmt_ = statement();
}
