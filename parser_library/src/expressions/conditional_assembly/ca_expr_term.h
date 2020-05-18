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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_TERM_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_TERM_H

#include <variant>
#include <vector>

#include "ca_expresssion.h"
#include "context/common_types.h"
#include "context/id_storage.h"
#include "context/ordinary_assembly/symbol_attributes.h"
#include "semantics/variable_symbol.h"
#include "ca_expr_policy.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_expr_list : public ca_expression
{
public:
    std::vector<ca_expr_ptr> expr_list;

    ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

private:
    template<typename EXPR_POLICY> ca_expr_ptr resolve(size_t& it, int priority);
    template<typename EXPR_POLICY> std::pair<int, ca_expr_ops> retrieve_binary_operator(size_t& it, bool& err);
};

class ca_string : public ca_expression
{
public:
    struct substring_t
    {
        ca_expr_ptr start, count;
        bool to_end;
        substring_t();
    };

    const semantics::concat_chain value;
    const ca_expr_ptr duplication_factor;
    const substring_t substring;

    ca_string(semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

class ca_var_sym : public ca_expression
{
public:
    const semantics::vs_ptr symbol;

    ca_var_sym(semantics::vs_ptr symbol, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

class ca_constant : public ca_expression
{
public:
    const context::A_t value;

    ca_constant(context::A_t value, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

class ca_symbol : public ca_expression
{
public:
    const context::id_index symbol;

    ca_symbol(context::id_index symbol, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;
};

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
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
