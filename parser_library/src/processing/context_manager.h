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

#ifndef PROCESSING_CONTEXT_MANAGER_H
#define PROCESSING_CONTEXT_MANAGER_H

#include "diagnosable_ctx.h"
#include "context/hlasm_context.h"
#include "workspace/parse_lib_provider.h"
#include "expressions/expression.h"
#include "expressions/evaluation_context.h"
#include "processing_format.h"
#include "antlr4-runtime.h"
#include "semantics/range_provider.h"


namespace hlasm_plugin {
namespace parser_library {
namespace processing {

//class wrapping context providing ranges, checks and diagnostics to hlasm_context
class context_manager : public diagnosable_ctx
{
public:
	using name_result = std::pair<bool, context::id_index>;

	//wrapped context
	context::hlasm_context& hlasm_ctx;

	context_manager(context::hlasm_context& hlasm_ctx);

	context::SET_t evaluate_expression(antlr4::ParserRuleContext* expr_context, expressions::evaluation_context eval_ctx) const;
	template <typename T>
	T evaluate_expression_to(antlr4::ParserRuleContext* expr_context, expressions::evaluation_context eval_ctx) const
	{
		return convert_to<T>(
			evaluate_expression(expr_context, eval_ctx),
			semantics::range_provider().get_range(expr_context)
			);
	}

	context::SET_t convert(context::SET_t source, context::SET_t_enum target_type, range value_range) const;
	template <typename T> 
	T convert_to(context::SET_t source, range value_range) const
	{
		auto tmp = convert(std::move(source), context::object_traits<T>::type_enum, value_range);

		if constexpr (std::is_same_v<T, context::A_t>)
			return tmp.access_a();
		if constexpr (std::is_same_v<T, context::B_t>)
			return tmp.access_b();
		if constexpr (std::is_same_v<T, context::C_t>)
			return std::move(tmp.access_c());
	}

	context::SET_t get_var_sym_value(const semantics::var_sym& symbol, expressions::evaluation_context eval_ctx) const;
	context::SET_t get_var_sym_value(context::id_index name, const expressions::expr_list& subscript, const range& symbol_range) const;

	name_result try_get_symbol_name(const semantics::var_sym* symbol, expressions::evaluation_context eval_ctx) const;
	name_result try_get_symbol_name(const std::string& symbol,range symbol_range) const;

	context::id_index concatenate(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;
	std::string concatenate_str(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;

	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain) const;
	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const;

	bool test_symbol_for_read(context::var_sym_ptr var, const expressions::expr_list& subscript, const range& symbol_range) const;

	virtual void collect_diags() const override;

private:
	context::macro_data_ptr create_macro_data(const semantics::concat_chain& chain, 
		const std::function<std::string(const semantics::concat_chain& chain)>& to_string) const;
};

}
}
}
#endif
