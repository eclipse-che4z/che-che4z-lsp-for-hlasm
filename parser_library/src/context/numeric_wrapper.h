#ifndef HLASMPLUGIN_PARSER_NUMERIC_WRAPPER_H
#define HLASMPLUGIN_PARSER_NUMERIC_WRAPPER_H
#include "expression.h"
#include "arithmetic_expression.h"
#include "logic_expression.h"

namespace hlasm_plugin
{
	namespace parser_library
	{
		namespace context
		{
			template<typename T>
			class arithmetic_logic_expr_wrapper {
				typename std::enable_if_t<std::is_base_of_v<expression, typename std::remove_const_t<typename std::remove_reference_t<T>>>, T>&& ref;
				arithmetic_logic_expr_wrapper() = delete;
				arithmetic_logic_expr_wrapper(T&& r)
					: ref(std::forward<T>(r)) {}
			public:
				arithmetic_logic_expr_wrapper(const arithmetic_logic_expr_wrapper<T>&) = default;
				arithmetic_logic_expr_wrapper(arithmetic_logic_expr_wrapper<T>&&) = default;
				static arithmetic_logic_expr_wrapper<T> wrap(T&& u) {
					return arithmetic_logic_expr_wrapper(std::forward<T>(u));
				}
				operator bool() const {
					return is_valid();
				}
				bool operator !() const {
					return !is_valid();
				}
				bool is_valid() const {
					if (ref.retype<arithmetic_expression>())
						return true;
					if (ref.retype<logic_expression>())
						return true;
					return false;
				}
				int32_t value() const {
					if (auto p = ref.retype<arithmetic_expression>()) {
						return p->get_value();
					}
					if (auto p = ref.retype<logic_expression>()) {
						return p->get_value();
					}
					throw std::bad_cast();
				}
			};

			template<typename U>
			static arithmetic_logic_expr_wrapper<U&&> al_wrap(U&& u) {
				return arithmetic_logic_expr_wrapper<U&&>::wrap(std::forward<U>(u));
			}
		} // namespace context
	} // namespace parser_library
} // namespace hlasm_plugin

#endif
