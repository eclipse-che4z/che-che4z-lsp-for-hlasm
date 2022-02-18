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

#include "context_manager.h"

#include "context/variables/system_variable.h"

namespace hlasm_plugin::parser_library::processing {

context_manager::context_manager(context::hlasm_context& hlasm_ctx)
    : diagnosable_ctx(hlasm_ctx)
    , eval_ctx_(nullptr)
    , hlasm_ctx(hlasm_ctx)
{}

context_manager::context_manager(const expressions::evaluation_context* eval_ctx)
    : diagnosable_ctx(eval_ctx->hlasm_ctx)
    , eval_ctx_(eval_ctx)
    , hlasm_ctx(eval_ctx->hlasm_ctx)
{}

context::SET_t context_manager::get_var_sym_value(
    context::id_index name, const std::vector<context::A_t>& subscript, range symbol_range) const
{
    auto var = hlasm_ctx.get_var_sym(name);

    bool ok = test_symbol_for_read(var, subscript, symbol_range);

    if (!ok)
        return context::SET_t();

    if (auto set_sym = var->access_set_symbol_base())
    {
        size_t idx = 0;

        if (subscript.empty())
        {
            switch (set_sym->type)
            {
                case context::SET_t_enum::A_TYPE:
                    return set_sym->access_set_symbol<context::A_t>()->get_value();
                case context::SET_t_enum::B_TYPE:
                    return set_sym->access_set_symbol<context::B_t>()->get_value();
                case context::SET_t_enum::C_TYPE:
                    return set_sym->access_set_symbol<context::C_t>()->get_value();
                default:
                    return context::SET_t();
            }
        }
        else
        {
            idx = (size_t)(subscript.front() - 1);

            switch (set_sym->type)
            {
                case context::SET_t_enum::A_TYPE:
                    return set_sym->access_set_symbol<context::A_t>()->get_value(idx);
                case context::SET_t_enum::B_TYPE:
                    return set_sym->access_set_symbol<context::B_t>()->get_value(idx);
                case context::SET_t_enum::C_TYPE:
                    return set_sym->access_set_symbol<context::C_t>()->get_value(idx);
                default:
                    return context::SET_t();
            }
        }
    }
    else if (auto mac_par = var->access_macro_param_base())
    {
        std::vector<size_t> tmp;
        for (auto& v : subscript)
        {
            tmp.push_back((size_t)v);
        }
        return mac_par->get_value(tmp);
    }
    return context::SET_t();
}

context::id_index context_manager::get_symbol_name(const std::string& symbol, range symbol_range) const
{
    auto tmp = hlasm_ctx.try_get_symbol_name(symbol);
    if (!tmp.first)
        add_diagnostic(diagnostic_op::error_E065(symbol_range));
    return tmp.second;
}

bool context_manager::test_symbol_for_read(
    const context::var_sym_ptr& var, const std::vector<context::A_t>& subscript, range symbol_range) const
{
    if (!var)
    {
        add_diagnostic(diagnostic_op::error_E010("variable", symbol_range)); // error - unknown name of variable
        return false;
    }

    if (auto set_sym = var->access_set_symbol_base())
    {
        if (subscript.size() > 1)
        {
            add_diagnostic(
                diagnostic_op::error_E020("variable symbol subscript", symbol_range)); // error - too many operands
            return false;
        }

        if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
        {
            add_diagnostic(
                diagnostic_op::error_E013("subscript error", symbol_range)); // error - inconsistent format of subcript
            return false;
        }

        if (!set_sym->is_scalar && (subscript.front() < 1))
        {
            add_diagnostic(diagnostic_op::error_E012(
                "subscript value has to be 1 or more", symbol_range)); // error - subscript is less than 1
            return false;
        }
    }
    else if (auto mac_par = var->access_macro_param_base())
    {
        for (size_t i = 0; i < subscript.size(); ++i)
        {
            if (subscript[i] < 1)
            {
                // if syslist and subscript = 0, ok
                if (i == 0 && subscript[i] == 0 && dynamic_cast<context::system_variable*>(mac_par))
                    continue;

                add_diagnostic(diagnostic_op::error_E012(
                    "subscript value has to be 1 or more", symbol_range)); // error - subscript is less than 1
                return false;
            }
        }
    }

    return true;
}

void context_manager::collect_diags() const {}

void context_manager::add_diagnostic(diagnostic_s diagnostic) const
{
    if (eval_ctx_)
        eval_ctx_->add_diagnostic(std::move(diagnostic));
    else
        diagnosable_ctx::add_diagnostic(std::move(diagnostic));
}

} // namespace hlasm_plugin::parser_library::processing
