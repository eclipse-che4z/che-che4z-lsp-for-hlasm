/*
 * Copyright (c) 2026 Broadcom.
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

#ifndef HLASMPARSER_UTILS_TEXT_CONVERTOR_H
#define HLASMPARSER_UTILS_TEXT_CONVERTOR_H

#include <string>
#include <string_view>

namespace hlasm_plugin::utils {

struct text_convertor
{
    virtual void from(std::string& dst, std::string_view src) const = 0;
    virtual void to(std::string& dst, std::string_view src) const = 0;

protected:
    ~text_convertor() = default;
};

class conversion_helper
{
    const text_convertor* tc;

public:
    explicit constexpr conversion_helper(const text_convertor* tc) noexcept
        : tc(tc)
    {}

    const conversion_helper& append_to(std::string& t, std::string_view s) const;
    const conversion_helper& append_from(std::string& t, std::string_view s) const;

    std::string convert_to(std::string&& s) const;
    std::string convert_from(std::string&& s) const;
    std::string convert_to(std::string_view s) const;
    std::string convert_from(std::string_view s) const;

    template<typename... S>
    std::string convert_to(S&&... s) const
    {
        std::string result;
        (append_to(result, static_cast<std::string_view>(s)), ...);
        return result;
    }
    template<typename... S>
    std::string convert_from(S&&... s) const
    {
        std::string result;
        (append_from(result, static_cast<std::string_view>(s)), ...);
        return result;
    }

    void inplace_to(std::string& s) const;
    void inplace_from(std::string& s) const;
};


} // namespace hlasm_plugin::utils

#endif
