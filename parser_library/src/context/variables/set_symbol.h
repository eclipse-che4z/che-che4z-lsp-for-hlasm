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

#ifndef CONTEXT_SET_SYMBOL_H
#define CONTEXT_SET_SYMBOL_H

#include <map>
#include <vector>

#include "variable.h"

namespace hlasm_plugin::parser_library::context {

template<typename T>
class set_symbol;

// base class for set_symbols
class set_symbol_base : public variable_symbol
{
public:
    // describes whether set symbol is scalar
    // when scalar, sublist notation is not allowed
    const bool is_scalar;

    // returns type of set symbol
    const SET_t_enum type;

    // casts this to specialized set symbol
    template<typename T>
    set_symbol<T>* access_set_symbol()
    {
        return (type == object_traits<T>::type_enum) ? static_cast<set_symbol<T>*>(this) : nullptr;
    }

    template<typename T>
    const set_symbol<T>* access_set_symbol() const
    {
        return (type == object_traits<T>::type_enum) ? static_cast<const set_symbol<T>*>(this) : nullptr;
    }

    virtual std::vector<A_t> keys() const = 0;

    bool can_read(
        std::span<const A_t> subscript, range symbol_range, diagnostic_consumer<diagnostic_op>& diags) const override;

protected:
    set_symbol_base(id_index name, bool is_scalar, SET_t_enum type);
};

// specialized set symbol holding data T (int = A_t, bool = B_t, std::string=C_t)
template<typename T>
class set_symbol final : public set_symbol_base
{
    static_assert(object_traits<T>::type_enum != SET_t_enum::UNDEF_TYPE, "Not a SET variable type.");

    // data holding this set_symbol
    // can be scalar or only array of scalars - no other nesting allowed
    std::map<A_t, T> data;

public:
    set_symbol(id_index name, bool is_scalar)
        : set_symbol_base(name, is_scalar, object_traits<T>::type_enum)
    {}

    // gets value from non scalar set symbol
    // if data at idx is not set or it does not exists, default is returned
    T get_value(A_t idx) const
    {
        if (is_scalar)
            return object_traits<T>::default_v();

        auto tmp = data.find(idx);
        if (tmp == data.end())
            return object_traits<T>::default_v();
        return tmp->second;
    }

    // gets value from scalar set symbol
    T get_value() const
    {
        if (!is_scalar)
            return object_traits<T>::default_v();

        auto tmp = data.find(0);
        if (tmp == data.end())
            return object_traits<T>::default_v();
        return tmp->second;
    }

    // sets value to scalar set symbol
    void set_value(T value) { data.insert_or_assign(0, std::move(value)); }

    // sets value to non scalar set symbol
    // any index can be accessed
    void set_value(T value, A_t idx)
    {
        if (is_scalar)
            data.insert_or_assign(0, std::move(value));
        else
            data.insert_or_assign(idx, std::move(value));
    }

    // reserves storage for the object value
    T& reserve_value() { return data[0]; }

    // reserves storage for the object value
    // any index can be accessed
    T& reserve_value(A_t idx)
    {
        if (is_scalar)
            return data[0];
        else
            return data[idx];
    }

    // N' attribute of the symbol
    A_t number(std::span<const A_t>) const override
    {
        return (A_t)(is_scalar || data.empty() ? 0 : data.rbegin()->first);
    }

    // K' attribute of the symbol
    A_t count(std::span<const A_t> offset) const override;

    std::vector<A_t> keys() const override
    {
        std::vector<A_t> keys;
        keys.reserve(data.size());
        for (const auto& [key, value] : data)
            keys.push_back(key);
        return keys;
    }

private:
    const T* get_data(std::span<const A_t> offset) const
    {
        if ((is_scalar && !offset.empty()) || (!is_scalar && offset.size() != 1))
            return nullptr;

        auto tmp_offs = is_scalar ? 0 : offset.front();

        auto it = data.find(tmp_offs);

        if (it == data.end())
            return nullptr;

        return &it->second;
    }
};

} // namespace hlasm_plugin::parser_library::context

#endif
