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

#include "utils/text_convertor.h"

namespace hlasm_plugin::utils {

const conversion_helper& conversion_helper::append_to(std::string& t, std::string_view s) const
{
    if (!tc)
        t.append(s);
    else
        tc->to(t, s);

    return *this;
}

const conversion_helper& conversion_helper::append_from(std::string& t, std::string_view s) const
{
    if (!tc)
        t.append(s);
    else
        tc->from(t, s);

    return *this;
}

std::string conversion_helper::convert_to(std::string&& s) const
{
    if (!tc)
        return std::move(s);

    std::string result;
    result.reserve(s.size());
    tc->to(result, s);
    return result;
}

std::string conversion_helper::convert_from(std::string&& s) const
{
    if (!tc)
        return std::move(s);

    std::string result;
    result.reserve(s.size());
    tc->from(result, s);
    return result;
}

std::string conversion_helper::convert_to(std::string_view s) const
{
    if (!tc)
        return std::string(s);

    std::string result;
    result.reserve(s.size());
    tc->to(result, s);
    return result;
}

std::string conversion_helper::convert_from(std::string_view s) const
{
    if (!tc)
        return std::string(s);

    std::string result;
    result.reserve(s.size());
    tc->from(result, s);
    return result;
}

void conversion_helper::inplace_to(std::string& s) const
{
    if (!tc)
        return;

    std::string result;
    result.reserve(s.size());
    tc->to(result, s);
    swap(result, s);
}

void conversion_helper::inplace_from(std::string& s) const
{
    if (!tc)
        return;

    std::string result;
    result.reserve(s.size());
    tc->from(result, s);
    swap(result, s);
}

} // namespace hlasm_plugin::utils
