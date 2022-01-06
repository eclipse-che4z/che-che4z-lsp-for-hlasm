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
#include <variant>

#include "diagnostic_consumer.h"

namespace hlasm_plugin::parser_library {
struct cics_preprocessor_options;
struct db2_preprocessor_options;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::processing {

using library_fetcher = std::function<std::optional<std::string>(std::string_view)>;

class preprocessor
{
public:
    virtual ~preprocessor() = default;

    virtual std::optional<std::string> generate_replacement(std::string_view& input, size_t& lineno) = 0;

    virtual bool finished() const = 0;

    static std::unique_ptr<preprocessor> create(
        const cics_preprocessor_options&, library_fetcher, diagnostic_op_consumer*);

    static std::unique_ptr<preprocessor> create(
        const db2_preprocessor_options&, library_fetcher, diagnostic_op_consumer*);
};
} // namespace hlasm_plugin::parser_library::processing

#endif
