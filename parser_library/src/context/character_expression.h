#ifndef HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H
#include "expression.h"
#include "logic_expression.h"
#include <string>

namespace hlasm_plugin
{
	namespace parser_library
	{
		namespace context
		{
			class expression;
			class character_expression;
			using char_ptr = std::unique_ptr<character_expression>;
			class character_expression : public expression
			{
			public:
				character_expression() = default;
				character_expression(character_expression &&) = default;
				character_expression(std::string);

				void append(std::string);
				char_ptr append(const char_ptr& arg) const;

				expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const;
				expr_ptr unary_operation(str_ref operation_name) const;

				const std::string &get_value() const;
				char_ptr substring(int32_t, expr_ref, expr_ref) const;
				std::string get_str_val() const override;

				static std::string num_to_hex(int32_t val);
				static char num_to_hex_char(int32_t val);
				static std::string num_to_ebcdic(int32_t val);
				static bool isalpha_hlasm(char c);
				static char hex_to_num(char c, size_t*);
			private:
				expr_ptr dclen() const;
				expr_ptr isbin() const;
				expr_ptr isdec() const;
				expr_ptr ishex() const;
				expr_ptr issym() const;
				expr_ptr b2c() const;
				expr_ptr b2d() const;
				expr_ptr b2x() const;
				expr_ptr c2b() const;
				expr_ptr c2d() const;
				expr_ptr c2x() const;
				expr_ptr d2b() const;
				expr_ptr d2c() const;
				expr_ptr d2x() const;
				expr_ptr dcval() const;
				expr_ptr dequote() const;
				expr_ptr double_quote() const;
				expr_ptr x2b() const;
				expr_ptr x2c() const;
				expr_ptr x2d() const;

				std::string value_ = "";
			};
		} // namespace context
	} // namespace parser_library
} // namespace hlasm_plugin

#endif
