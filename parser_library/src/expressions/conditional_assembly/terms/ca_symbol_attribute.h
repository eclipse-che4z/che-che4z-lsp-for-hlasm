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

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression attributed ordinary symbol
class ca_symbol_attribute : public ca_expression
{
    // variant of ordinary symbol, variable symbol(, literal TODO)
    using ca_attr_variant_t = std::variant<context::id_index, semantics::vs_ptr>;

public:
    const context::data_attr_kind attribute;
    ca_attr_variant_t symbol;

    ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute, range expr_range);
    ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    // if expr contains a symbol as a first term, the rest of the string is thrown away
    // used for L'I'S'T' reference of variable symbol
    static void try_extract_leading_symbol(std::string& expr);

private:
    context::SET_t get_ordsym_attr_value(context::id_index name, const evaluation_context& eval_ctx) const;
    context::SET_t retrieve_value(const context::symbol* ord_symbol, const evaluation_context& eval_ctx) const;

    context::SET_t evaluate_ordsym(context::id_index symbol, const evaluation_context& eval_ctx) const;
    context::SET_t evaluate_varsym(const semantics::vs_ptr& symbol, const evaluation_context& eval_ctx) const;
    context::SET_t evaluate_substituted(context::id_index var_name,
        std::vector<context::A_t> expr_subscript,
        range var_range,
        const evaluation_context& eval_ctx) const;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
