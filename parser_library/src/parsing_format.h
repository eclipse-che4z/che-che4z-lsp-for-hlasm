#pragma once

#include "context/instruction.h"

namespace hlasm_plugin {
namespace parser_library {

//helper class for determining parsing format of statements
struct parsing_format
{
	parsing_format() : in_lookahead(false), defered_operands(false), no_operands(false), alt_format(false) {}

	//prepares for next statement, in_lookahead variable is preserved because it remains between statements
	void reset()
	{
		defered_operands = false;
		no_operands = false;
		alt_format = false;
	}

	void expect_macro()
	{
		no_operands = false;
		operand_type = context::instruction_type::MAC;
		alt_format = true;
	}
	void expect_CA(bool has_no_ops)
	{
		no_operands = has_no_ops;
		operand_type = context::instruction_type::CA;
		alt_format = true;
	}
	void expect_defered()
	{
		defered_operands = true;
	}

	bool in_lookahead;						//I. statement format
	bool defered_operands;					//II. operands and remarks format
	bool no_operands;						//III. i. operands format
	bool alt_format;						//IV. ii. operands format
	context::instruction_type operand_type;	//V. operand format
};

}
}