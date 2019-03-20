#ifndef SEMANTICS_STATEMENTFIELDS_H
#define SEMANTICS_STATEMENTFIELDS_H
#include "../context/id_storage.h"
#include "../context/instruction.h"
#include "operand.h"
#include "concatenation.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class label_type
{
	ORD, SEQ, VAR, MAC, CONC, EMPTY
};

//struct holding info about label field
struct label_semantic_info
{
	label_semantic_info() : type(label_type::EMPTY) {}

	label_semantic_info(const label_semantic_info& label): type(label.type),name(label.name), sequence_symbol(label.sequence_symbol), range(label.range), variable_symbol(label.variable_symbol)
	{
		concatenation.insert(concatenation.end(), make_clone_iterator(label.concatenation.begin()), make_clone_iterator(label.concatenation.end()));
	}

	label_semantic_info(label_semantic_info&&) = default;
	label_semantic_info& operator=(label_semantic_info&&) = default;

	label_type type;

	std::string name;
	concat_chain concatenation;
	seq_sym sequence_symbol;
	var_sym variable_symbol;

	symbol_range range;
};

enum class instr_semantic_type
{
	ORD,CONC,EMPTY
};

//struct holding info about instruction field
struct instruction_semantic_info
{
	instruction_semantic_info() : type(instr_semantic_type::EMPTY) {}

	instruction_semantic_info(const instruction_semantic_info& instruction) : type(instruction.type), ordinary_name(instruction.ordinary_name), range(instruction.range)
	{
		model_name.insert(model_name.end(), make_clone_iterator(instruction.model_name.begin()), make_clone_iterator(instruction.model_name.end()));
	}

	instruction_semantic_info& operator=(const instruction_semantic_info& instruction)
	{
		type = instruction.type;
		ordinary_name= instruction.ordinary_name;
		range = instruction.range;
		model_name.insert(model_name.end(), make_clone_iterator(instruction.model_name.begin()), make_clone_iterator(instruction.model_name.end()));

		return *this;
	}

	instruction_semantic_info(instruction_semantic_info&&) = default;
	instruction_semantic_info& operator=(instruction_semantic_info&&) = default;

	instr_semantic_type type;

	std::string ordinary_name;

	concat_chain model_name;

	symbol_range range;
};

//struct holding info about operand and remark field
struct operand_remark_semantic_info
{
	operand_remark_semantic_info() :is_defered(false) {}

	operand_remark_semantic_info(const operand_remark_semantic_info& op_rem):is_defered(op_rem.is_defered), range(op_rem.range), remarks(op_rem.remarks)
	{
		operands.insert(operands.end(), make_clone_iterator(op_rem.operands.begin()), make_clone_iterator(op_rem.operands.end()));

		defered_field.insert(defered_field.end(), make_clone_iterator(op_rem.defered_field.begin()), make_clone_iterator(op_rem.defered_field.end()));
	}

	operand_remark_semantic_info(operand_remark_semantic_info&&) = default;

	operand_remark_semantic_info& operator=(operand_remark_semantic_info&&) = default;

	std::vector<operand_ptr> operands;

	std::vector<symbol_range> remarks;

	bool is_defered;

	concat_chain defered_field;

	symbol_range range;
};

//struct holding info about whole instruction statement, whole logical line
struct statement
{
	label_semantic_info label_info;

	instruction_semantic_info instr_info;

	operand_remark_semantic_info op_rem_info;

	symbol_range range;
};

using statement_block = std::vector<statement>;

}
}
}
#endif
