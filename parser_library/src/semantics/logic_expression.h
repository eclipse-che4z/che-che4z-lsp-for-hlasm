#ifndef HLASMPLUGIN_PARSER_HLASMLEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMLEXPRESSION_H
#include "expression.h"
#include "arithmetic_expression.h"
#include <string>

namespace hlasm_plugin {
	namespace parser_library {
		namespace semantics {
			class arithmetic_expression;
			class expression;
			class logic_expression : public expression
			{
			public:
				virtual ~logic_expression() = default;
				logic_expression() = default;
				logic_expression(bool);
				logic_expression(const logic_expression& expr);
				logic_expression(logic_expression&&) = default;
				logic_expression& operator=(logic_expression&&) = default;
				logic_expression(int32_t);

				expr_ptr to_arith() const;

				expr_ptr unary_operation(str_ref operation_name) const override;
				expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const override;

				int32_t get_numeric_value() const override;
				
				std::string get_str_val() const override;
				bool get_value() const;
				SET_t get_set_value() const override;

				virtual expr_ptr operator+(expression_ref) const override;
				virtual expr_ptr operator-(expression_ref) const override;
				virtual expr_ptr operator*(expression_ref) const override;
				virtual expr_ptr operator/(expression_ref) const override;
				virtual expr_ptr operator|(expression_ref) const override;
				virtual expr_ptr operator&(expression_ref) const override;
				virtual expr_ptr operator+() const override;
				virtual expr_ptr operator-() const override;
			private:
				bool value_ = false;

			};
		}
	}
}

#endif
