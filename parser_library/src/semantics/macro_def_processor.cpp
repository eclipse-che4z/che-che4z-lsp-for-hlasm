#include "macro_def_processor.h"
#include "processing_manager.h"

using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library::semantics;

macro_def_processor::macro_def_processor(processing_manager& mngr) : statement_processor(mngr, std::bind(&macro_def_processor::init_table, this, std::placeholders::_1)) {}

void hlasm_plugin::parser_library::semantics::macro_def_processor::set_start_info(start_info_ptr info)
{
	expecting_prototype_ = true;
	macro_nest_ = 0;
	macro_def_.clear();

	auto macro_info = dynamic_cast<macro_def_info*>(info.get());

	if (!macro_info)
		throw std::invalid_argument("bad type of start_info");

	start_info_ = std::make_unique<macro_def_info>(std::move(*macro_info));
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_instruction(instruction_semantic_info instruction)
{
	if (expecting_prototype_)
	{
		if (instruction.type != instr_semantic_type::EMPTY)
		{
			auto name = instruction.type == instr_semantic_type::ORD ? std::move(instruction.ordinary_name) : ctx_mngr().to_string(std::move(instruction.model_name));
			curr_op_code_ = op_code_info(ctx_mngr().get_id(std::move(name)), instruction_type::MAC, false);
		}
		else
		{
			//WARNING empty line after MACRO instr, macro name set to ASPACE -- use startinfo.macro_range				TODO
			curr_op_code_ = op_code_info(ctx_mngr().get_id("ASPACE"), instruction_type::MAC, false);
		}

		parser().format.expect_macro();
	}
	else
	{
		curr_instruction_ = instruction;

		if (instruction.type != instr_semantic_type::CONC)
		{
			curr_op_code_ = ctx_mngr().get_opcode_info(std::move(instruction.ordinary_name));
		}
		else //instruction contains variable symbol, can not be deduced now
		{
			parser().format.expect_defered();
			curr_op_code_ = op_code_info();
			return;
		}

		//only CA can not be affected by OPSYN, therefore about operand format we can be sure only in CA 
		if (curr_op_code_.type == instruction_type::CA)
		{
			parser().format.expect_CA(curr_op_code_.has_no_ops);
		}
		else
		{
			parser().format.expect_defered();
		}
	}
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_statement(statement statement)
{
	curr_statement_ = std::move(statement);

	if (expecting_prototype_)
	{
		process_prototype();
	}
	else
	{
		auto it = process_table.find(curr_op_code_.op_code);
		if (it != process_table.end())
			it->second();
		else
			store_statement();
	}

	if (is_last_line() && macro_nest_ != 0)
		;//ERR missing MEND																				TODO
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::collect_diags() const {}

process_table_t hlasm_plugin::parser_library::semantics::macro_def_processor::init_table(hlasm_context& ctx)
{
	process_table_t table;
	table.emplace(ctx.ids.add("MACRO"),
		std::bind(&macro_def_processor::process_MACRO, this));
	table.emplace(ctx.ids.add("MEND"),
		std::bind(&macro_def_processor::process_MEND, this));
	//	TODO
	//	process_table.emplace(ctx().context.ids.add("COPY"),
	//		std::bind(&macro_def_processor::process_COPY, this));

	return table;
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_prototype()
{
	expecting_prototype_ = false;

	definition_begin_ln_ = curr_statement_.range.end_ln + 1;

	std::vector<id_index> param_names;

	//label processing
	if (curr_statement_.label_info.type == label_type::VAR)
	{
		auto& var = curr_statement_.label_info.variable_symbol;
		if (var.created || var.subscript.size() != 0)
			;//err bad name param																	TODO
		else
		{
			prototype_.label = ctx_mngr().get_id(std::move(var.name));
			param_names.push_back(prototype_.label);
		}
	}
	else if (curr_statement_.label_info.type != label_type::EMPTY)
		;//warn or err, on label field of prototype macro definition is something else than varsym	TODO

	//instr
	prototype_.name = curr_op_code_.op_code;

	//ops
	for (auto& op : curr_statement_.op_rem_info.operands)
	{
		if (!op || op->type == operand_type::EMPTY)
		{
			prototype_.symbolic_params.emplace_back(nullptr, nullptr);
			continue;
		}

		auto tmp = op->access_mac_op();
		assert(tmp);

		clear_concat_chain(tmp->chain);

		if (tmp->chain.size() == 1 && tmp->chain[0]->get_type() == concat_type::VAR) //if operand is varsym
		{
			auto var_id = ctx_mngr().get_id(std::move(tmp->chain[0]->access_var()->name));

			if (!tmp->chain[0]->access_var()->subscript.empty())
				;//err macro param in prototype statement has subscript								TODO

			if (std::find(param_names.begin(), param_names.end(), var_id) != param_names.end())
			{
				add_diagnostic(diagnostic_s::error_E011("", "Symbolic parameter", context_manager::convert_range(tmp->range)));
				prototype_.symbolic_params.emplace_back(nullptr, nullptr);
			}
			else
			{
				param_names.push_back(var_id);
				prototype_.symbolic_params.emplace_back(nullptr, var_id);
			}
		}
		else if (tmp->chain.size() == 0)											//if operand is empty
		{
			prototype_.symbolic_params.emplace_back(nullptr, nullptr);
		}
		else if (tmp->chain.size() > 1)
		{
			if (tmp->chain[0]->get_type() == concat_type::VAR && tmp->chain[1]->get_type() == concat_type::EQUALS)	//if operand is in form of key param
			{
				auto var_id = ctx_mngr().get_id(std::move(tmp->chain[0]->access_var()->name));

				if (!tmp->chain[0]->access_var()->subscript.empty())
					;//err macro param in prototype statement has subscript									TODO

				if (std::find(param_names.begin(), param_names.end(), var_id) != param_names.end())
				{
					add_diagnostic(diagnostic_s::error_E011("", "Symbolic parameter", context_manager::convert_range(tmp->range)));
				}
				else
				{
					param_names.push_back(var_id);

					tmp->chain.erase(tmp->chain.begin());
					tmp->chain.erase(tmp->chain.begin());

					prototype_.symbolic_params.emplace_back(ctx_mngr().create_macro_data(std::move(tmp->chain)), var_id);
				}
			}
			else
				;//err invalid parameter in macro prototype statement										TODO
		}
	}


}

void hlasm_plugin::parser_library::semantics::macro_def_processor::store_statement()
{
	curr_statement_.instr_info = curr_instruction_;

	if (curr_statement_.label_info.type == label_type::SEQ)
		curr_statement_.label_info.sequence_symbol.location.line = macro_def_.size();

	if (curr_op_code_.type == instruction_type::CA)
	{
		for (auto& op : curr_statement_.op_rem_info.operands)
		{
			if (!op || op->type == operand_type::EMPTY) continue;

			auto ca_op = op->access_ca_op();
			if (ca_op->kind == ca_operand_kind::BRANCH_EXPR || ca_op->kind == ca_operand_kind::BRANCH_SIMPLE)
				ca_op->sequence_symbol.location.line = macro_def_.size();
		}
	}

	macro_def_.push_back(std::move(curr_statement_));
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_MACRO()
{
	store_statement();
	++macro_nest_;
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_COPY()
{
	//TODO
}

void hlasm_plugin::parser_library::semantics::macro_def_processor::process_MEND()
{
	store_statement();
	if (macro_nest_-- > 0)
		return;

	ctx_mngr().ctx().add_macro(prototype_.name, prototype_.label, std::move(prototype_.symbolic_params), std::move(macro_def_), file_name(), { start_info_->macro_range.begin_ln,definition_begin_ln_ });
	finish();
}
