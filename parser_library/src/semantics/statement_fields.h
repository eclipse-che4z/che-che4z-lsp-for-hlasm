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
	ORD, SEQ, VAR, MAC, SUB, EMPTY
};

//struct holding info about label field
struct label_semantic_info
{
	label_semantic_info() : type(label_type::EMPTY) {}

	label_type type;

	std::string name;
	seq_sym sequence_symbol;
	var_sym variable_symbol;
};

//struct holding info about instruction field
struct instruction_semantic_info
{
	instruction_semantic_info() :id(nullptr) {}

	context::id_index id;
	context::instruction_type type;
	bool has_alt_format;
	bool has_no_ops;
};

//struct holding info about operand and remark field
struct operand_remark_semantic_info
{
	operand_remark_semantic_info() :substituted(false) {}

	std::vector<operand_ptr> operands;
	std::vector<symbol_range> remarks;

	bool substituted;

	std::vector<operand_ptr> substituted_operands;
	std::vector<symbol_range> substituted_remarks;

	auto& actual_operands() { return substituted ? substituted_operands : operands; }
};

//struct holding info about whole instruction statement, whole logical line
struct statement
{
	label_semantic_info label_info;

	instruction_semantic_info instr_info;

	operand_remark_semantic_info op_rem_info;

	symbol_range range;
};

}
}
}
#endif
