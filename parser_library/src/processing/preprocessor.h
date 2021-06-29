/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_H
#define HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_H

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "diagnostic.h"

namespace hlasm_plugin::parser_library::config {
struct db2_preprocessor;
} // namespace hlasm_plugin::parser_library::config

namespace hlasm_plugin::parser_library::processing {

using library_fetched = std::function<std::optional<std::string>(std::string_view)>;
using diag_reported = std::function<void(diagnostic_op)>;

class preprocessor
{
public:
    virtual ~preprocessor() = default;

    virtual bool fill_buffer(std::string_view& input, size_t lineno, std::deque<std::string>& buffer) = 0;

    static std::unique_ptr<preprocessor> create(const config::db2_preprocessor&, library_fetched, diag_reported);
};
} // namespace hlasm_plugin::parser_library::processing

#endif
