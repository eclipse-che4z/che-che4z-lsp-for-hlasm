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

#include "context/variables/macro_param.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging {

variable generate_macro_param_variable(const context::macro_param_base& param, std::vector<int32_t> index)
{
    variable result {
        .name = index.empty() ? "&" + param.id.to_string() : std::to_string(index.back()),
        .value = index.empty() ? param.macro_param_base::get_value() : param.get_value(index),
        .type = set_type::C_TYPE,
    };

    if (const auto index_range = param.index_range(index); index_range)
    {
        result.values = index_range->first > index_range->second
            ? std::function([]() { return std::vector<variable>(); })
            : std::function([index = std::move(index), r = index_range.value(), &param]() {
                  std::vector<variable> vars;

                  auto child_index = index;
                  child_index.push_back(0);

                  vars.reserve(r.second - r.first);
                  for (long long i = r.first; i <= r.second; ++i)
                  {
                      child_index.back() = (context::A_t)i;
                      vars.emplace_back(generate_macro_param_variable(param, child_index));
                  }

                  return vars;
              });
    }

    return result;
}

} // namespace hlasm_plugin::parser_library::debugging
