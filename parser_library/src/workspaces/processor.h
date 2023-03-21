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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_H

#include <set>
#include <unordered_map>
#include <vector>

#include "diagnosable.h"
#include "preprocessor_options.h"
#include "protocol.h"
#include "semantics/highlighting_info.h"

namespace hlasm_plugin::utils::resource {
class resource_location;
}

namespace hlasm_plugin::parser_library {
struct analyzing_context;
struct asm_option;
struct fade_message_s;
class virtual_file_monitor;

namespace lsp {
class lsp_context;
}

namespace processing {
struct hit_count_entry;
} // namespace processing

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::workspaces {
struct library_data;
class parse_lib_provider;
using file_location = utils::resource::resource_location;

// Interface that represents an object that can be parsed.
// The only implementor is processor_file
class processor : public virtual diagnosable
{
public:
    // starts parser with new (empty) context
    virtual bool parse(parse_lib_provider&, asm_option, std::vector<preprocessor_options>, virtual_file_monitor*) = 0;
    // starts parser with in the context of parameter
    virtual bool parse_macro(parse_lib_provider&, analyzing_context, library_data) = 0;

protected:
    ~processor() = default;
};

// Interface that represents a file that can be parsed.
class processor_file : public processor
{
public:
    virtual const std::set<utils::resource::resource_location>& dependencies() = 0;
    virtual const semantics::lines_info& get_hl_info() = 0;
    virtual const lsp::lsp_context* get_lsp_context() const = 0;
    virtual const std::set<utils::resource::resource_location>& files_to_close() = 0;
    virtual const performance_metrics& get_metrics() = 0;
    virtual void erase_unused_cache_entries() = 0;
    virtual bool has_opencode_lsp_info() const = 0;
    virtual bool has_macro_lsp_info() const = 0;
    virtual const std::vector<fade_message_s>& fade_messages() const = 0;
    virtual const std::unordered_map<utils::resource::resource_location,
        processing::hit_count_entry,
        utils::resource::resource_location_hasher>&
    hit_count_opencode_map() const = 0;
    virtual const std::unordered_map<utils::resource::resource_location,
        processing::hit_count_entry,
        utils::resource::resource_location_hasher>&
    hit_count_macro_map() const = 0;
    virtual const file_location& get_location() const = 0;
    virtual bool current_version() const = 0;

protected:
    ~processor_file() = default;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif
