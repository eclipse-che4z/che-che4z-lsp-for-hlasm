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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_H

#include <string>
#include <vector>

#include "range.h"

namespace hlasm_plugin::utils::resource {
class resource_location;
} // namespace hlasm_plugin::utils::resource

namespace hlasm_plugin::parser_library {

enum class diagnostic_severity
{
    error = 1,
    warning = 2,
    info = 3,
    hint = 4,
    unspecified = 5
};

enum class diagnostic_tag
{
    none = 0,
    unnecessary = 1 << 0,
    deprecated = 1 << 1,
};

struct range_uri
{
    range_uri() = default;
    range_uri(std::string uri, range range)
        : uri(std::move(uri))
        , rang(range)
    {}

    std::string uri;
    range rang;
};

// Represents related info (location with message) of LSP diagnostic.
class diagnostic_related_info
{
public:
    diagnostic_related_info() = default;
    diagnostic_related_info(range_uri location, std::string message)
        : location(std::move(location))
        , message(std::move(message))
    {}
    range_uri location;
    std::string message;
};

// Represents a LSP diagnostic.
struct diagnostic
{
    diagnostic() = default;

    diagnostic(std::string file_uri, range range, std::string code, std::string message)
        : file_uri(std::move(file_uri))
        , diag_range(range)
        , code(std::move(code))
        , message(std::move(message))
    {}
    diagnostic(std::string file_uri,
        range range,
        diagnostic_severity severity,
        std::string code,
        std::string message,
        std::vector<diagnostic_related_info> related,
        diagnostic_tag tag)
        : file_uri(std::move(file_uri))
        , diag_range(range)
        , severity(severity)
        , code(std::move(code))
        , message(std::move(message))
        , related(std::move(related))
        , tag(tag)
    {}

    std::string file_uri;
    range diag_range;
    diagnostic_severity severity = diagnostic_severity::unspecified;
    std::string code;
    std::string source = "HLASM Plugin";
    std::string message;
    std::vector<diagnostic_related_info> related;
    diagnostic_tag tag = diagnostic_tag::none;
};

} // namespace hlasm_plugin::parser_library

#endif
