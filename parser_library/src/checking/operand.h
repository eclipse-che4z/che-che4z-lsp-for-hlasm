#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_OPERAND_H

#include <memory>
#include "shared/range.h"

namespace hlasm_plugin::parser_library::checking
{

class operand
{
public:
	operand() = default;
	operand(range operand_range) : operand_range(operand_range) {}
	range operand_range;

	virtual ~operand() = default;
};

using check_op_ptr = std::unique_ptr<operand>;

class asm_operand : public virtual operand
{
public:
	asm_operand() = default;
	virtual ~asm_operand() = default;
};


}

#endif
