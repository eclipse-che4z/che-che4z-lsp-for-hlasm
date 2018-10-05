#include "semantic_analyzer.h"
#include "semantic_analyzer.h"
#include "semantic_analyzer.h"
#include "semantic_analyzer.h"
#include "semantic_analyzer.h"
#include "../include/shared/lexer.h"
#include "../include/shared/token_stream.h"
#include "../generated/hlasmparser.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;

hlasm_plugin::parser_library::context::hlasm_context& hlasm_plugin::parser_library::semantics::semantic_analyzer::context()
{
	return *ctx_;
}

hlasm_plugin::parser_library::lexer * hlasm_plugin::parser_library::semantics::semantic_analyzer::lexer()
{
	return lexer_;
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::initialize(std::shared_ptr<context::hlasm_context> ctx_init, hlasm_plugin::parser_library::lexer* lexer_init)
{
	this->ctx_ = std::move(ctx_init);
	this->lexer_ = lexer_init;
	ord_processor_ = std::make_unique<ordinary_processor>(*this);
	look_processor_ = std::make_unique<lookahead_processor>(*this);
	lookahead_failed_ = false;
	init_instr();
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::initialize(const semantic_analyzer & analyzer)
{
	ctx_ = analyzer.ctx_;
	curr_statement_.instr_info = analyzer.curr_statement_.instr_info;
	lookahead_failed_ = false;
}

const label_semantic_info& hlasm_plugin::parser_library::semantics::semantic_analyzer::current_label()
{
	return curr_statement_.label_info;
}

const instruction_semantic_info& hlasm_plugin::parser_library::semantics::semantic_analyzer::current_instruction()
{
	return curr_statement_.instr_info;
}

const operand_remark_semantic_info& hlasm_plugin::parser_library::semantics::semantic_analyzer::current_operands_and_remarks()
{
	return curr_statement_.op_rem_info;
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_label_field()
{
	curr_statement_.label_info.type = label_type::EMPTY;
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_label_field(std::string label)
{
	curr_statement_.label_info.type = label_type::MAC;
	curr_statement_.label_info.name = std::move(label);
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_label_field(seq_sym sequence_symbol)
{
	curr_statement_.label_info.type = label_type::SEQ;
	curr_statement_.label_info.sequence_symbol = std::move(sequence_symbol);
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_label_field(std::string label, antlr4::ParserRuleContext * parser_ctx)
{
	//recognise, whether label consists only of ORDSYMBOL token
	if (parser_ctx->getStart() == parser_ctx->getStop() && parser_ctx->getStart()->getType() == lexer::Tokens::ORDSYMBOL)
	{
		curr_statement_.label_info.type = label_type::ORD;
		curr_statement_.label_info.name = std::move(label);
	}
	//recognise, whether label consists of DOT ORDSYMBOL tokens, so it is sequence symbol
	else if (parser_ctx->children.size() == 2 && parser_ctx->getStart()->getType() == lexer::Tokens::DOT && parser_ctx->getStop()->getType() == lexer::Tokens::ORDSYMBOL)
	{
		curr_statement_.label_info.type = label_type::SEQ;
		curr_statement_.label_info.sequence_symbol = { std::move(label.erase(0, 1)), { parser_ctx->getStart()->getLine(), parser_ctx->getStart()->getStartIndex() } };
	}
	else
		set_label_field(std::move(label));
}

void clear_conc_list_(std::vector<concat_point_ptr>& list)
{
	size_t offset = 0;
	for (size_t i = 0; i < list.size(); ++i)
	{
		if (list[i] && !(list[i]->get_type() == concat_type::STR && list[i]->access_str()->value.empty())) //if not empty ptr and not empty string
			list[offset++] = std::move(list[i]);
	}

	list.resize(offset);
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_label_field(std::vector<concat_point_ptr> label)
{
	clear_conc_list_(label);
	if (label.size() == 1 && label[0]->get_type() == concat_type::VAR)
	{
		curr_statement_.label_info.type = label_type::VAR;
		curr_statement_.label_info.variable_symbol = std::move(*label[0]->access_var());
	}
	else
	{
		curr_statement_.label_info.type = label_type::SUB;
		curr_statement_.label_info.name = concatenate(std::move(label));
	}
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_instruction_field(std::string instr)
{
	//if contains space

	auto id = ctx_->ids.add(instr);

	auto target = ctx_->get_mnemonic_opcode(id);

	if (target == ctx_->empty_id)
	{
		//ERROR no op code
		curr_statement_.instr_info.has_no_ops = true;
		curr_statement_.instr_info.id = target;
	}
	else if (target != nullptr)
		id = target;

	//TODO check for :MAC :ASM

	auto instr_info = instructions_.find(id);

	if (instr_info != instructions_.end())
	{
		curr_statement_.instr_info.id = id;
		curr_statement_.instr_info.type = instr_info->second.type;
		curr_statement_.instr_info.has_no_ops = instr_info->second.no_ops;
		curr_statement_.instr_info.has_alt_format = instr_info->second.type == instruction_type::CA || instr_info->second.type == instruction_type::MAC;
	}
	else
	{
		//TODO check for mac libs
		curr_statement_.instr_info.id = id;
		curr_statement_.instr_info.type = instruction_type::MAC;
		curr_statement_.instr_info.has_no_ops = false;
		curr_statement_.instr_info.has_alt_format = true;
	}
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_instruction_field(std::vector<concat_point_ptr> instr)
{
	auto str = concatenate(std::move(instr));

	if (str.find(' ') != std::string::npos)
	{
		//error
	}

	set_instruction_field(std::move(str));
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_operand_remark_field(std::string name)
{
	//TODO add to asm operand
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_operand_remark_field(std::vector<operand_ptr> operands, std::vector<symbol_range> remarks)
{
	if (curr_statement_.instr_info.type != instruction_type::CA && curr_statement_.instr_info.type != instruction_type::MAC &&
		std::find_if(operands.begin(), operands.end(), [](auto& op) {return op && op->type() == operand_type::MODEL; }) != operands.end())
	{
		std::string op_rem_field;
		for (size_t i = 0; i < operands.size(); ++i)
		{
			if (auto model = operands[i]->access_model_op())
				op_rem_field.append(concatenate(std::move(model->conc_list)));
			else if (auto subs = operands[i]->access_subs_op())
				op_rem_field.append(subs->get_text());

			if (i != operands.size() - 1)
				op_rem_field.push_back(',');
		}

		//substituting and reparsing
		input_source input(std::move(op_rem_field));
		hlasm_plugin::parser_library::lexer lex(&input);
		lex.set_unlimited_line(true);
		token_stream tokens(&lex);
		generated::hlasmparser parser(&tokens);
		parser.initialize(*this);
		parser.removeErrorListeners();
		auto res = parser.operands_model();
		curr_statement_.op_rem_info.substituted_operands = std::move(res->line.operands);
		curr_statement_.op_rem_info.substituted_remarks = std::move(res->line.remarks);
		curr_statement_.op_rem_info.substituted = true;
	}

	curr_statement_.op_rem_info.operands = std::move(operands);
	curr_statement_.op_rem_info.remarks = std::move(remarks);
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::set_statement_field(symbol_range range)
{
	curr_statement_.range = range;
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::process_statement()
{
	if (lookahead_failed_)
	{
		curr_statement_ = statement();
		lookahead_failed_ = false;
		return;
	}

	if (lookahead_.active)
		look_processor_->process_current_statement();
	else
		ord_processor_->process_current_statement();
	curr_statement_ = statement();
}

bool hlasm_plugin::parser_library::semantics::semantic_analyzer::in_lookahead()
{
	return lookahead_.active;
}

set_type semantic_analyzer::get_var_sym_value(id_index id, const std::vector<expr_ptr>& subscript, symbol_range range)
{
	auto var = ctx_->get_var_sym(id);
	if (!var)
	{
		//ERROR no var like that
		return set_type();
	}

	if (auto set_sym = var->access_set_symbol_base())
	{
		if (subscript.size() > 1)
		{
			//error too much operands
			return set_type();
		}

		if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
		{
			//error inconsistent subscript
			return set_type();
		}

		if (!set_sym->is_scalar && (!subscript[0] || subscript[0]->get_numeric_value() < 1))
		{
			//error subscript lesser than 1
			return set_type();
		}

		size_t idx = 0;
		if (subscript.empty())
		{
			if (auto seta = set_sym->access_set_symbol<A_t>())
				return seta->get_value();
			else if (auto setb = set_sym->access_set_symbol<B_t>())
				return setb->get_value();
			else if (auto setc = set_sym->access_set_symbol<C_t>())
				return setc->get_value();
		}
		else
		{
			idx = (size_t)(subscript[0] ? subscript[0]->get_numeric_value() - 1 : 0);

			if (auto seta = set_sym->access_set_symbol<A_t>())
				return seta->get_value(idx);
			else if (auto setb = set_sym->access_set_symbol<B_t>())
				return setb->get_value(idx);
			else if (auto setc = set_sym->access_set_symbol<C_t>())
				return setc->get_value(idx);
		}
	}
	else if (auto mac_par = var->access_macro_param_base())
	{
		std::vector<size_t> tmp;
		for (auto& e : subscript)
		{
			if (!e || e->get_numeric_value() < 1)
			{
				//error subscript
				return set_type();
			}
			tmp.push_back((size_t)(e->get_numeric_value() - 1));
		}
		return mac_par->get_value(tmp);
	}
	return set_type();
}

id_index hlasm_plugin::parser_library::semantics::semantic_analyzer::get_id(std::string name)
{
	//ERR check if valid id
	return ctx_->ids.add(std::move(name));
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::jump(sequence_symbol target)
{
	auto label = ctx_->get_sequence_symbol(target.name);
	if (!label)
	{
		lookahead_ = look_ahead(target, look_ahead::la_type::SEQ);
	}
	else
	{
		ctx_->decrement_branch_counter();

		if (ctx_->get_branch_counter() < 0)
		{
			//TODO action
			return;
		}

		jump_in_statements(label.location);
	}
}

std::string semantic_analyzer::concatenate(std::vector<concat_point_ptr>&& conc_list)
{
	std::string result;
	bool last_was_var = false;

	for (auto& point : conc_list)
	{
		if (!point) continue;
		switch (point->get_type())
		{
		case concat_type::STR:
			last_was_var = false;
			result.append(to_string(point->access_str()));
			break;
		case concat_type::DOT:
			if (last_was_var) continue;
			last_was_var = false;
			result.append(to_string(point->access_dot()));
			break;
		case concat_type::VAR:
			last_was_var = true;
			result.append(to_string(point->access_var()));
			break;
		case concat_type::SUB:
			last_was_var = false;
			result.append(to_string(point->access_sub()));
			break;
		default:
			break;
		}
	}

	return result;
}

std::string hlasm_plugin::parser_library::semantics::semantic_analyzer::to_string(char_str* str)
{
	return std::move(str->value);
}

std::string hlasm_plugin::parser_library::semantics::semantic_analyzer::to_string(var_sym* vs)
{
	auto id = ctx_->ids.add(std::move(vs->name));
	return get_var_sym_value(id, vs->subscript, vs->range).to<C_t>();
}

std::string hlasm_plugin::parser_library::semantics::semantic_analyzer::to_string(dot*)
{
	return ".";
}

std::string hlasm_plugin::parser_library::semantics::semantic_analyzer::to_string(sublist* sublist)
{
	std::string ret("(");
	for (size_t i = 0; i < sublist->list.size(); ++i)
	{
		if (!sublist->list[i]) continue;

		switch (sublist->list[i]->get_type())
		{
		case concat_type::STR:
			ret.append(to_string(sublist->list[i]->access_str()));
		case concat_type::DOT:
			ret.append(to_string(sublist->list[i]->access_dot()));
		case concat_type::VAR:
			ret.append(to_string(sublist->list[i]->access_var()));
		case concat_type::SUB:
			ret.append(to_string(sublist->list[i]->access_sub()));
		}
		if (i != sublist->list.size() - 1) ret.append(",");
	}
	ret.append(")");
	return ret;
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::init_instr()
{
	if (instructions_.size() != 0)
		return;

	for (size_t i = 0; i < instruction::machine_instructions.size(); ++i)
	{
		auto id = ctx_->ids.add(instruction::machine_instructions[i].name);
		instructions_.insert({ id, { instruction_type::MACH,instruction::machine_instructions[i].operands_count == 0,i } });
	}
	for (size_t i = 0; i < instruction::assembler_instructions.size(); ++i)
	{
		auto id = ctx_->ids.add(instruction::assembler_instructions[i].name);
		if (*id == "DC" || *id == "DS")
			instructions_.insert({ id, { instruction_type::DAT,instruction::assembler_instructions[i].max_operands == 0,i } });
		else
			instructions_.insert({ id, { instruction_type::ASM,instruction::assembler_instructions[i].max_operands == 0,i } });
	}
	for (size_t i = 0; i < instruction::ca_instructions.size(); ++i)
	{
		auto id = ctx_->ids.add(instruction::ca_instructions[i]);
		instructions_.insert({ id, { instruction_type::CA,instruction::ca_instructions[i] == "ANOP",i } });
	}
}

void hlasm_plugin::parser_library::semantics::semantic_analyzer::jump_in_statements(context::location location)
{
	lexer_->rewind_input(location.offset, location.line);
}
