#ifndef SEMANTICS_OPERAND_H
#define SEMANTICS_OPERAND_H
#include <vector>
#include <functional>
#include "semantic_objects.h"
#include "concatenation.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class operand_type
{
	MACH, MAC, ASM, CA, DAT, MODEL, EMPTY, UNDEF
};

struct model_operand;
struct ca_operand;
struct substituable_operand;

struct operand;
using operand_ptr = std::unique_ptr<operand>;

struct op_rem
{
	std::vector<operand_ptr> operands;
	std::vector<symbol_range> remarks;
};

//struct representing operand of instruction
struct operand
{
	symbol_range range;

	model_operand* access_model_op();
	ca_operand* access_ca_op();
	substituable_operand* access_subs_op();

	virtual operand_type type() const;
	virtual ~operand() = default;
};

struct empty_operand : public operand
{
	operand_type type() const override;
};

//operand that contains variable symbol thus is 'model operand'
struct model_operand : public operand
{
	model_operand(std::vector<concat_point_ptr> conc_list);

	std::vector<concat_point_ptr> conc_list;

	operand_type type() const override;
};

//this operand can contibute in evaluation of model operands, creating substituted operand and remark field
struct substituable_operand : public operand
{
	substituable_operand(std::function<std::string()> get_text);
	std::function<std::string()> get_text;
};

struct machine_operand : public substituable_operand
{
	machine_operand(std::function<std::string()> get_text);
	operand_type type() const override;
};

struct assembler_operand : public substituable_operand
{
	assembler_operand(std::function<std::string()> get_text);
	operand_type type() const override;
};

enum class ca_operand_kind
{
	VAR, EXPR, BRANCH_SIMPLE, BRANCH_EXPR
};

struct ca_operand : public operand
{
	expr_ptr expression;
	seq_sym seqence_symbol;
	var_sym vs;

	const ca_operand_kind kind;

	ca_operand(seq_sym seqence_symbol);

	ca_operand(var_sym vs);

	ca_operand(seq_sym seqence_symbol, expr_ptr expression);

	ca_operand(expr_ptr expression);

	operand_type type() const override;
};

}
}
}
#endif
