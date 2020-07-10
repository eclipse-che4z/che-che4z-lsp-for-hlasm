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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_SYMBOL_ATTRIBUTE_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_SYMBOL_ATTRIBUTE_H

#include <variant>

#include "../ca_expression.h"
#include "semantics/variable_symbol.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_symbol_attribute : public ca_expression
{
    // variant of ordinary symbol, variable symbol(, literal TODO)
    using ca_attr_variant_t = std::variant<context::id_index, semantics::vs_ptr>;

public:
    const context::data_attr_kind attribute;
    ca_attr_variant_t symbol;

    ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute, range expr_range);
    ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const override;

private:
    context::SET_t get_ordsym_attr_value(context::id_index name, evaluation_context& eval_ctx) const;
    context::SET_t retrieve_value(context::symbol* ord_symbol, evaluation_context& eval_ctx) const;

    context::SET_t evaluate_ordsym(context::id_index symbol, evaluation_context& eval_ctx) const;
    context::SET_t evaluate_varsym(const semantics::vs_ptr& symbol, evaluation_context& eval_ctx) const;
};


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
