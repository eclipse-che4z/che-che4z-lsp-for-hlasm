#ifndef HLASMPLUGIN_PARSER_HLASMAEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMAEXPRESSION_H
#include "expression.h"
#include <string>

namespace hlasm_plugin {
	namespace parser_library {
		namespace expressions {
			class logic_expression;
			class arithmetic_expression;
			using arith_ptr = std::unique_ptr<arithmetic_expression>;
			class arithmetic_expression : public expression
			{
			public:
				virtual ~arithmetic_expression() = default;
				arithmetic_expression() = default;
				arithmetic_expression(int32_t);
				arithmetic_expression(const arithmetic_expression& expr);
				arithmetic_expression(arithmetic_expression&&) = default;
				arithmetic_expression& operator=(arithmetic_expression&&) = default;
				static expr_ptr from_string(const std::string& option, const std::string& value, bool dbcs);

				static expr_ptr c2arith(const std::string & value);
				static expr_ptr g2arith(const std::string & value, bool dbcs = false);

				virtual expr_ptr operator+(expression_ref) const override;
				virtual expr_ptr operator-(expression_ref) const override;
				virtual expr_ptr operator*(expression_ref) const override;
				virtual expr_ptr operator/(expression_ref) const override;
				virtual expr_ptr operator+() const override;
				virtual expr_ptr operator-() const override;

				virtual int32_t get_numeric_value() const override;

				virtual expr_ptr unary_operation(str_ref operation_name) const override;
				expr_ptr binary_operation(str_ref o, expr_ref arg2) const;
				static expr_ptr from_string(const std::string&, int base);
				context::SET_t get_set_value() const override;
				int32_t get_value() const;
				virtual std::string get_str_val() const override;

			private:
				int32_t value_ = 0;
			};
		}
	}
}

#endif
