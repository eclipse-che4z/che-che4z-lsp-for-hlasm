#ifndef HLASMPLUGIN_PARSER_HLASMEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMEXPRESSION_H
#include <memory>
#include <string>
#include <deque>
#include "antlr4-runtime.h"
#include "../context/common_types.h"
#include "../diagnosable.h"
#include "../error_messages.h"

namespace hlasm_plugin
{
	namespace parser_library
	{
		namespace expressions
		{
			class expression;
			using expr_ptr = std::shared_ptr<expression>;
			using expr_list = std::vector< expr_ptr>;
			using str_ref = const std::string &;
			using expr_ref = const expr_ptr &;
			using expression_ref = const expression &;
			class character_expression;
			class arithmetic_expression;
			class expression
			{
			public:
				std::unique_ptr<diagnostic_op> diag;
				bool has_error() const { return diag != nullptr; }
				virtual ~expression() = default;

				virtual expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const;
				virtual expr_ptr unary_operation(str_ref operation_name) const;

				static expr_ptr resolve_ord_symbol(str_ref symbol);
				static expr_ptr evaluate(std::deque<expr_ptr> exprs);
				static expr_ptr self_defining_term(str_ref type, str_ref val, bool dbcs);

				virtual expr_ptr operator+(expression_ref) const;
				virtual expr_ptr operator-(expression_ref) const;
				virtual expr_ptr operator*(expression_ref) const;
				virtual expr_ptr operator/(expression_ref) const;
				virtual expr_ptr operator|(expression_ref) const;
				virtual expr_ptr operator&(expression_ref) const;
				virtual expr_ptr operator+() const;
				virtual expr_ptr operator-() const;

				virtual int32_t get_numeric_value() const;

				virtual context::SET_t get_set_value() const;
				
				virtual bool is_keyword() const { return false; }
				virtual std::string get_str_val() const = 0;

				template <typename T>
				T *retype()
				{
					return dynamic_cast<T *>(this);
				}

				template <typename T>
				const T *retype() const
				{
					return dynamic_cast<const T *>(this);
				}


				expression(expression&&) = default;
				expression& operator=(expression&&) = default;

			protected:
				expression() = default;
				void copy_diag(expr_ref o);
				void copy_diag(const expression* o);
				void copy_diag(const expression& o);

				template <typename T>
				static typename std::shared_ptr<T> default_expr_with_error(std::unique_ptr<diagnostic_op> diag)
				{
					auto ex = std::make_shared<T>();
					ex->diag = std::move(diag);
					return ex;
				}

				template<typename T>
				static typename std::shared_ptr<T> test_and_copy_error(const expression* e)
				{
					if (e->has_error())
					{
						auto ex = std::make_shared<T>();
						ex->copy_diag(e);
						return ex;
					}
					return std::unique_ptr<T>();
				}

				template<typename T>
				static typename std::shared_ptr<T> test_and_copy_error(const expression& e)
				{
					if (e.has_error())
					{
						auto ex = std::make_shared<T>();
						ex->copy_diag(e);
						return ex;
					}
					return std::unique_ptr<T>();
				}

				static expr_ptr evaluate_term(std::deque<expr_ptr>& exprs, uint8_t priority, size_t& operator_count);
				static expr_ptr evaluate_factor(std::deque<expr_ptr>& exprs, size_t& operator_count);
			};
		} // namespace semantics
	} // namespace parser_library
} // namespace hlasm_plugin

#define make_arith(val) std::make_shared<arithmetic_expression>(val)
#define make_logic(val) std::make_shared<logic_expression>(val)
#define make_char(val) std::make_shared<character_expression>(val)

#define copy_return_on_error(arg, type) \
	do \
	{ \
		auto te = test_and_copy_error<type>(arg); \
		if (te != nullptr) return te; \
	} \
	while(0)

#define copy_return_on_error_binary(arg2, type) \
	do \
	{ \
		copy_return_on_error(this, type); \
		copy_return_on_error(arg2, type); \
	} \
	while(0)

#endif
