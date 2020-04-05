/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSER_HLASMAEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMAEXPRESSION_H
#include "expression.h"
#include <string>
#include <string_view>

namespace hlasm_plugin {
	namespace parser_library {
		namespace expressions {
			class logic_expression;
			class arithmetic_expression;
			using arith_ptr = std::unique_ptr<arithmetic_expression>;
			/**
			 * logic for arithmetic expression and operations on this type
			 * */
			class arithmetic_expression : public expression
			{
			public:
				virtual ~arithmetic_expression() = default;
				arithmetic_expression() = default;
				arithmetic_expression(int32_t);
				arithmetic_expression(const arithmetic_expression& expr);
				arithmetic_expression(arithmetic_expression&&) = default;
				arithmetic_expression& operator=(arithmetic_expression&&) = default;
				static expr_ptr from_string(const std::string& option, const std::string_view& value, bool dbcs);
				static expr_ptr from_string(const std::string_view& value, bool dbcs);

				//EBCDIC string interpret as number (1 char = 1B)
				static expr_ptr c2arith(const std::string& value);
				//double byte interpret as number
				static expr_ptr g2arith(const std::string& value, bool dbcs = false);

				/**
				 * all operations involving arguments check for errors
				 * in all arguments immediately before accessing their values
				 * 
				 * if any argument contains an error, it is copied
				 * and an erroneous expression (meaning with en error)
				 * is returned
				 * 
				 * see: copy_return_on_error and copy_return_on_error_binary
				 * */

				virtual expr_ptr operator+(expression_ref) const override;
				virtual expr_ptr operator-(expression_ref) const override;
				virtual expr_ptr operator*(expression_ref) const override;
				virtual expr_ptr operator/(expression_ref) const override;
				virtual expr_ptr operator+() const override;
				virtual expr_ptr operator-() const override;

				virtual int32_t get_numeric_value() const override;

				virtual expr_ptr unary_operation(str_ref operation_name) const override;
				virtual expr_ptr binary_operation(str_ref o, expr_ref arg2) const override;
				static expr_ptr from_string(const std::string_view&, int base);
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
