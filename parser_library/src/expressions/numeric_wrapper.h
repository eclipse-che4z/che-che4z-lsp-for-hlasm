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

#ifndef HLASMPLUGIN_PARSER_NUMERIC_WRAPPER_H
#define HLASMPLUGIN_PARSER_NUMERIC_WRAPPER_H
#include "arithmetic_expression.h"
#include "expression.h"
#include "logic_expression.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {
/**
 * Wrapper for logic and arithmetic expressions
 * HLASM supports expressions with mixed operand types
 * logic expression is then converted to arithmetic
 * */
template<typename T> class arithmetic_logic_expr_wrapper
{
    typename std::enable_if_t<
        std::is_base_of_v<expression, typename std::remove_const_t<typename std::remove_reference_t<T>>>,
        T>&& ref;
    arithmetic_logic_expr_wrapper() = delete;
    arithmetic_logic_expr_wrapper(T&& r)
        : ref(std::forward<T>(r))
    {}

public:
    arithmetic_logic_expr_wrapper(const arithmetic_logic_expr_wrapper<T>&) = default;
    arithmetic_logic_expr_wrapper(arithmetic_logic_expr_wrapper<T>&&) = default;
    static arithmetic_logic_expr_wrapper<T> wrap(T&& u) { return arithmetic_logic_expr_wrapper(std::forward<T>(u)); }
    operator bool() const { return is_valid(); }
    bool operator!() const { return !is_valid(); }
    bool is_valid() const
    {
        if (ref.template retype<arithmetic_expression>())
            return true;
        if (ref.template retype<logic_expression>())
            return true;
        return false;
    }
    int32_t value() const
    {
        if (auto p = ref.template retype<arithmetic_expression>())
        {
            return p->get_value();
        }
        if (auto p = ref.template retype<logic_expression>())
        {
            return p->get_value();
        }
        throw std::bad_cast();
    }
};

template<typename U> static arithmetic_logic_expr_wrapper<U&&> al_wrap(U&& u)
{
    return arithmetic_logic_expr_wrapper<U&&>::wrap(std::forward<U>(u));
}
} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin

#endif
