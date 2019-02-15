#ifndef HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H
#include "expression.h"
#include "logic_expression.h"
#include <string>

namespace hlasm_plugin
{
	namespace parser_library
	{
		namespace semantics
		{
			class expression;
			class character_expression;
			using char_ptr = std::shared_ptr<character_expression>;
			class character_expression : public expression
			{
			public:
				character_expression() = default;
				character_expression(character_expression &&) = default;
				character_expression(std::string);

				void append(std::string);
				char_ptr append(const char_ptr& arg) const;
				char_ptr append(const character_expression* arg) const;

				expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const;
				expr_ptr unary_operation(str_ref operation_name) const;

				const std::string &get_value() const;

				template<typename T>
				char_ptr substring(int32_t dupl, const T& s, const T& e) const
				{
					if (dupl < 0)
						return default_expr_with_error<character_expression>
						(error_messages::ec01());

					int32_t start = 0;
					int32_t len = static_cast<int32_t>(value_.length());

					if (s != nullptr)
						start = s->get_numeric_value() - 1;


					if (e != nullptr)
						len = e->get_numeric_value();

					if (start < 0 || len < 0)
						return default_expr_with_error<character_expression>
						(error_messages::ec02());

					if ((size_t)start > value_.size())
						return default_expr_with_error<character_expression>
						(error_messages::ec02());

					auto value = value_.substr(start, len);

					if (dupl > 1)
					{
						value.reserve(dupl * value.length());
						auto val = value;
						for (int32_t i = 1; i < dupl; ++i)
							value.append(val);
					}

					return std::make_shared<character_expression>(std::move(value));
				}

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
		} // namespace semantics
	} // namespace parser_library
} // namespace hlasm_plugin

#endif
