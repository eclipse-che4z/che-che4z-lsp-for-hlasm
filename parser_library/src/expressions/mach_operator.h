#ifndef HLASMPLUGIN_PARSERLIBRARY_MACH_OPERATOR_H
#define HLASMPLUGIN_PARSERLIBRARY_MACH_OPERATOR_H

#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions
{

template <typename T>
class mach_expr_binary final : public mach_expression
{
	mach_expr_ptr left_;
	mach_expr_ptr right_;
public:

	mach_expr_binary(mach_expr_ptr left, mach_expr_ptr right, range rng)
		: mach_expression(rng),
		left_(std::move(left)), right_(std::move(right))
	{
		//text = left_->move_text() + T::sign_char() + right_->move_text();
	}

	context::dependency_holder get_dependencies(mach_evaluate_info info) const override;

	virtual value_t evaluate(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override
	{
		left_->fill_location_counter(addr);
		right_->fill_location_counter(std::move(addr));
	}

	void collect_diags() const override
	{
		collect_diags_from_child(*left_);
		collect_diags_from_child(*right_);
	}

};


template <typename T>
class mach_expr_unary final : public mach_expression
{
	mach_expr_ptr child_;
public:

	mach_expr_unary(mach_expr_ptr child, range rng)
		: mach_expression(rng), child_(std::move(child))
	{
		//text = T::sign_char_begin() + child_->move_text() + T::sign_char_end();
	}

	context::dependency_holder get_dependencies(mach_evaluate_info info) const override;

	virtual void fill_location_counter(context::address addr) override
	{
		child_->fill_location_counter(std::move(addr));
	}

	virtual value_t evaluate(mach_evaluate_info info) const override;

	void collect_diags() const override
	{
		collect_diags_from_child(*child_);
	}

};

struct add
{
	static std::string sign_char()
	{
		return "+";
	}
	static std::string sign_char_begin()
	{
		return "+";
	}
	static std::string sign_char_end()
	{
		return "";
	}
};

struct sub
{
	static std::string sign_char()
	{
		return "-";
	}
	static std::string sign_char_begin()
	{
		return "-";
	}
	static std::string sign_char_end()
	{
		return "";
	}
};

struct mul
{
	static std::string sign_char()
	{
		return "*";
	}
};

struct div
{
	static std::string sign_char()
	{
		return "/";
	}
};

struct par
{
	static std::string sign_char_begin()
	{
		return "(";
	}
	static std::string sign_char_end()
	{
		return ")";
	}
};



template<>
inline mach_expression::value_t mach_expr_binary<add>::evaluate(mach_evaluate_info info) const
{
	return left_->evaluate(info) + right_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_binary<sub>::evaluate(mach_evaluate_info info) const
{
	return left_->evaluate(info) - right_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_binary<mul>::evaluate(mach_evaluate_info info) const
{
	auto left_res = left_->evaluate(info);
	auto right_res = right_->evaluate(info);

	if (!(left_res.value_kind() == context::symbol_kind::ABS && right_res.value_kind() == context::symbol_kind::ABS) &&
		left_res.value_kind() != context::symbol_kind::UNDEF && right_res.value_kind() != context::symbol_kind::UNDEF)
		add_diagnostic(diagnostic_s::error_ME002(get_range()));
		
		
	return left_res * right_res;
}

template<>
inline mach_expression::value_t mach_expr_binary<div>::evaluate(mach_evaluate_info info) const
{
	auto left_res = left_->evaluate(info);
	auto right_res = right_->evaluate(info);

	// division by zero
	if (right_res.value_kind() == context::symbol_kind::ABS && right_res.get_abs() == 0)
		return 0;

	if (!(left_res.value_kind() == context::symbol_kind::ABS && right_res.value_kind() == context::symbol_kind::ABS) &&
		left_res.value_kind() != context::symbol_kind::UNDEF && right_res.value_kind() != context::symbol_kind::UNDEF)
		add_diagnostic(diagnostic_s::error_ME002(get_range()));

	return left_res * right_res;
}

template<>
inline mach_expression::value_t mach_expr_unary<add>::evaluate(mach_evaluate_info info) const
{
	return child_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_unary<sub>::evaluate(mach_evaluate_info info) const
{
	return -child_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_unary<par>::evaluate(mach_evaluate_info info) const
{
	return child_->evaluate(info);
}

template<>
inline context::dependency_holder mach_expr_binary<add>::get_dependencies(mach_evaluate_info info) const
{
	return left_->get_dependencies(info) + right_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_binary<sub>::get_dependencies(mach_evaluate_info info) const
{
	return left_->get_dependencies(info) - right_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_binary<mul>::get_dependencies(mach_evaluate_info info) const
{
	return left_->get_dependencies(info) * right_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_binary<div>::get_dependencies(mach_evaluate_info info) const
{
	return left_->get_dependencies(info) / right_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_unary<add>::get_dependencies(mach_evaluate_info info) const
{
	return child_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_unary<sub>::get_dependencies(mach_evaluate_info info) const
{
	return context::dependency_holder() - child_->get_dependencies(info);
}

template<>
inline context::dependency_holder mach_expr_unary<par>::get_dependencies(mach_evaluate_info info) const
{
	return child_->get_dependencies(info);
}

}

#endif