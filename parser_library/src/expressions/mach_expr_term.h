#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_MACH_EXPR_TERM_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_MACH_EXPR_TERM_H

#include "mach_expression.h"
#include "../context/id_storage.h"

namespace hlasm_plugin::parser_library::expressions
{

class mach_expr_constant : public mach_expression
{
	value_t value_;
public:
	mach_expr_constant(std::string value_text, range rng);
	mach_expr_constant(int value, range rng);

	context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	virtual value_t evaluate(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override;

	void collect_diags() const override {}
};



class mach_expr_symbol : public mach_expression
{
	
public:
	mach_expr_symbol(context::id_index value, range rng);

	context::id_index value;

	static mach_expr_ptr from_id(context::id_index id, range rng);

	context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	virtual value_t evaluate(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override;

	void collect_diags() const override {}
};

class mach_expr_location_counter : public mach_expression
{
public:
	std::optional<context::address> location_counter;

	mach_expr_location_counter(range rng);

	context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	virtual value_t evaluate(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override;

	void collect_diags() const override {}
};



class mach_expr_self_def : public mach_expression
{
	value_t value_;
public:
	mach_expr_self_def(std::string option, std::string value, range rng);

	static mach_expr_ptr from_id(std::string id, range rng);

	context::dependency_holder get_dependencies(context::dependency_solver& solver) const override;

	virtual value_t evaluate(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override;

	void collect_diags() const override {}
};

}

#endif