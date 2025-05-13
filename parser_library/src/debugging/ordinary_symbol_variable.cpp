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

#include "context/ordinary_assembly/symbol.h"
#include "debug_types.h"
#include "ebcdic_encoding.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging {
namespace {
const std::string empty_string = "";
const std::string undef_string = "UNDEF";
const std::string reloc_string = "RELOC";
const std::string complex_string = "COMPLEX";

std::string get_string_value(const context::symbol& symbol)
{
    using enum context::symbol_value_kind;
    switch (symbol.kind())
    {
        case ABS:
            return std::to_string(symbol.value().get_abs());

        case RELOC:
            if (symbol.value().get_reloc().is_complex())
                return complex_string;
            else
                return reloc_string;

        case UNDEF:
            return undef_string;

        default:
            return empty_string;
    }
}
} // namespace

variable generate_ordinary_symbol_variable(const context::symbol& symbol)
{
    using enum context::data_attr_kind;
    variable result = {
        .name = symbol.name().to_string(),
        .value = get_string_value(symbol),
    };

    if (const auto& attr = symbol.attributes();
        attr.is_defined(L) || attr.is_defined(I) || attr.is_defined(S) || attr.is_defined(T))
    {
        result.values = [&symbol]() {
            std::vector<variable> vars;

            const auto& attr = symbol.attributes();
            if (attr.is_defined(I))
                vars.emplace_back(generate_attribute_variable("I", std::to_string(attr.get_attribute_value(I))));
            if (attr.is_defined(L))
                vars.emplace_back(generate_attribute_variable("L", std::to_string(attr.get_attribute_value(L))));
            if (attr.is_defined(S))
                vars.emplace_back(generate_attribute_variable("S", std::to_string(attr.get_attribute_value(S))));
            if (attr.is_defined(T))
                vars.emplace_back(generate_attribute_variable(
                    "T", ebcdic_encoding::to_ascii((unsigned char)attr.get_attribute_value(T))));

            return vars;
        };
    }

    return result;
}

} // namespace hlasm_plugin::parser_library::debugging
