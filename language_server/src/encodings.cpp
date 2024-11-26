/*
 * Copyright (c) 2024 Broadcom.
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

#include "encodings.h"

#include <cstdlib>
#include <numeric>

#include "server_options.h"
#include "utils/content_loader.h"

namespace hlasm_plugin::language_server {

constexpr auto iso8859_1_table = []() {
    utils::resource::translation_table result {};
    std::iota(result.begin(), result.end(), u'\0');
    return result;
}();


const utils::resource::translation_table* get_input_encoding(encodings e)
{
    using enum encodings;
    switch (e)
    {
        case none:
            return nullptr;

        case iso8859_1:
            return &iso8859_1_table;

        default:
            std::abort();
    }
}
} // namespace hlasm_plugin::language_server
