#include "data_definition.h"
#include "mach_expr_term.h"
#include <stdexcept>
#include <set>

#include <assert.h>

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;


data_definition data_definition::create(std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin)
{
	parser p(std::move(format), std::move(exprs), std::move(nominal), begin);
	return p.parse();
}

void insert_deps(context::dependency_holder& into, context::dependency_solver& solver, const context::dependable * from)
{
	if (from)
		into = into + from->get_dependencies(solver);
}

context::dependency_holder data_definition::get_dependencies(context::dependency_solver& solver) const
{
	context::dependency_holder conjuction;
	
	insert_deps(conjuction, solver, nominal_value.get());
	insert_deps(conjuction, solver, dupl_factor.get());
	insert_deps(conjuction, solver, length.get());
	insert_deps(conjuction, solver, scale.get());
	insert_deps(conjuction, solver, exponent.get());
	return conjuction;
}

void data_definition::collect_diags() const {}

data_definition::parser::parser(std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin)
	: format_(std::move(format)), exprs_(std::move(exprs)), nominal_(std::move(nominal)), pos_(begin), p_(0), exprs_i_(0) {}

bool is_number_char(char c)
{
	return isdigit(c) || c == '-';
}

size_t data_definition::parser::get_number_end(size_t begin)
{
	size_t i = begin;
	while (i < format_.size() && is_number_char(format_[i]))
		++i;
	return i;
}

//parses a number that begins on index begin in format_ string. Leaves index of the first character after the number in begin.
std::optional<int> data_definition::parser::parse_number()
{	
	std::optional<int> parsed;
	try
	{
		size_t pos;
		parsed = std::stoi(format_.substr(p_), &pos);
		p_ += pos;
		pos_.column += pos;
		return parsed;
	}
	catch (std::out_of_range &)
	{
		size_t end = get_number_end(p_); //Integer out of range
		position new_pos = { pos_.line, pos_.column + (end - p_) };
		result_.add_diagnostic(diagnostic_op::error_D001({ pos_, new_pos }));
		p_ = end;
		pos_ = new_pos;
		return std::nullopt;
	}
	catch (std::invalid_argument&)
	{
		size_t end = get_number_end(p_); //Expected an integer
		position new_pos = { pos_.line, pos_.column + (end - p_) };
		result_.add_diagnostic(diagnostic_op::error_D002({ pos_, new_pos }));
		p_ = end;
		pos_ = new_pos;
		return std::nullopt;
	}
}



void data_definition::parser::update_position(const mach_expression& e)
{
	pos_ = e.get_range().end;
	pos_.column += 1;
}
void data_definition::parser::update_position_by_one()
{
	++pos_.column;
}

void data_definition::parser::parse_duplication_factor()
{
	if (is_number_char(format_[0])) // duplication factor is present
	{
		position old_pos = pos_;
		auto dupl_factor_num = parse_number();
		
		if (dupl_factor_num)
			result_.dupl_factor = std::make_unique<mach_expr_constant>(*dupl_factor_num, range(old_pos, pos_));
		else
			result_.dupl_factor = std::make_unique<mach_expr_constant>(1, range(old_pos, pos_));
	}
	else if (format_[0] == *data_definition::expr_placeholder)
	{//duplication factor as expression
		result_.dupl_factor = move_next_expression();
	}
	else
	{
		result_.dupl_factor = std::make_unique<mach_expr_constant>(1, range(pos_, pos_));
	}
}

bool is_type_extension(char ch)
{
	static std::set<char> type_extensions({'A', 'E', 'U', 'H', 'B', 'D', 'Q', 'Y'});
	return type_extensions.find(ch) != type_extensions.end();
}

bool is_modifier_or_prog(char ch)
{
	return ch == 'P' || ch == 'L' || ch == 'S' || ch == 'E';
}

mach_expr_ptr data_definition::parser::move_next_expression()
{
	assert(exprs_i_ < exprs_.size());
	auto res = std::move(exprs_[exprs_i_]);
	++exprs_i_;
	update_position(*res);
	++p_;

	return std::move(res);
}

mach_expr_ptr data_definition::parser::parse_modifier_num_or_expr()
{
	if (is_number_char(format_[p_]))
	{
		position old_pos = pos_;
		auto modifier = parse_number();

		if (modifier)
			return std::make_unique<mach_expr_constant>(*modifier, range(old_pos, pos_));
		//else leave expr null
	}
	else if (format_[p_] == expr_placeholder[0])
	{
		return move_next_expression();
	}
	else if (format_[p_] == nominal_placeholder[0])
	{
		auto exprs = nominal_->access_exprs();
		if (exprs && exprs->exprs.size() == 1)
		{
			//it is possible, that a modifier has been parsed as nominal value,
			//if nominal value is not present at all and duplication factor is 0
			++p_;
			update_position(*exprs->exprs[0]);
			nominal_parsed_ = true;
			return std::move(exprs->exprs[0]);
			
		}
		else
		{
			result_.add_diagnostic(diagnostic_op::error_D003({ pos_, {pos_.line, pos_.column + 1} }));
			++p_;
			//no need to update pos_, nominal value (if present) is the last character of the format string
		}
	}
	return nullptr;
}

void data_definition::parser::assign_expr_to_modifier(char modifier, mach_expr_ptr expr)
{
	switch (modifier)
	{
	case 'P':
		result_.program_type = std::move(expr);
		break;
	case 'L':
		result_.length = std::move(expr);
		break;
	case 'S':
		result_.scale = std::move(expr);
		break;
	case 'E':
		result_.exponent = std::move(expr);
		break;
	default:
		assert(false);
	}
}



void data_definition::parser::parse_modifier()
{
	//we assume, that format_[p_] determines one of modifiers PLSE
	position begin_pos = pos_;
	char modifier = format_[p_++];
	if (p_ >= format_.size())
	{
		//expected something after modifier character
		result_.add_diagnostic(diagnostic_op::error_D003({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));
		return;
	}

	update_position_by_one();

	if (format_[p_] == '.')
	{
		++p_;
		
		if (modifier == 'L')
			result_.length_type = length_type::BIT;
		else
			result_.add_diagnostic(diagnostic_op::error_D005({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));

		update_position_by_one();

		if (p_ >= format_.size())
		{
			//expected something after modifier character
			result_.add_diagnostic(diagnostic_op::error_D003({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));
			return;
		}
	}
	mach_expr_ptr expr = parse_modifier_num_or_expr();
	
	size_t rem_pos = remaining_modifiers_.find(modifier);
	if (rem_pos == std::string::npos)//wrong order
		result_.add_diagnostic(diagnostic_op::error_D004({begin_pos, pos_}));
	else
		remaining_modifiers_ = remaining_modifiers_.substr(rem_pos + 1);

	assign_expr_to_modifier(modifier, std::move(expr));
}

data_definition data_definition::parser::parse()
{
	parse_duplication_factor();

	result_.type = format_[p_++];
	result_.type_range = { pos_, { pos_.line, pos_.column + 1 } };
	update_position_by_one();
	

	if (p_ >= format_.size())
		return std::move(result_);
	
	if (is_type_extension(format_[p_]))
	{
		result_.extension = format_[p_];
		result_.type_range = { pos_, { pos_.line, pos_.column + 1 } };
		++p_;
		update_position_by_one();
	}

	for (size_t i = 0; i < 5 && p_ < format_.size(); ++i)
	{
		if (is_modifier_or_prog(format_[p_]))
		{
			parse_modifier();
		}
		else if (format_[p_] == nominal_placeholder[0])
		{
			result_.nominal_value = std::move(nominal_);
			++p_;
		}
		else
		{
			result_.add_diagnostic(diagnostic_op::error_D006({ pos_, {pos_.line, pos_.column + 1} }));
			++p_;
			update_position_by_one();
		}
	}


	return std::move(result_);
}