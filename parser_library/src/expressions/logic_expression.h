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

#ifndef HLASMPLUGIN_PARSER_HLASMLEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMLEXPRESSION_H
#include "expression.h"
#include "arithmetic_expression.h"
#include <string>

namespace hlasm_plugin {
	namespace parser_library {
		namespace expressions {
			class arithmetic_expression;
			class expression;
			/**
			 * wraps logic and value of logic expression
			 * */
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

				expr_ptr unary_operation(str_ref operation_name) const override;
				expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const override;

				int32_t get_numeric_value() const override;
				
				std::string get_str_val() const override;
				bool get_value() const;
				context::SET_t get_set_value() const override;

				/**
				 * all operations are in INT32
				 * but we compute them in INT64
				 * and checking for overflow -> we return error
				 * 
				 * second operand is always converted to arithmetic expression
				 * */

				virtual expr_ptr operator+(expression_ref) const override;
				virtual expr_ptr operator-(expression_ref) const override;
				virtual expr_ptr operator*(expression_ref) const override;
				virtual expr_ptr operator/(expression_ref) const override;
				virtual expr_ptr operator+() const override;
				virtual expr_ptr operator-() const override;

				/**
				 * logical expressions
				 * return logical value
				 * */
				virtual expr_ptr operator|(expression_ref) const override;
				virtual expr_ptr operator&(expression_ref) const override;
			private:
				bool value_ = false;

			};
		}
	}
}

#endif
