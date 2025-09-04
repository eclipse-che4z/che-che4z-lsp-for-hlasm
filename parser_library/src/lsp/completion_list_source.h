/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LSP_COMPLETION_LIST_SOURCE_H
#define HLASMPLUGIN_PARSERLIBRARY_LSP_COMPLETION_LIST_SOURCE_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace hlasm_plugin::parser_library {
namespace context {
class id_index;
class macro_definition;
class section;
struct opencode_sequence_symbol;
struct macro_sequence_symbol;
class symbol;
} // namespace context
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::lsp {

class lsp_context;
struct macro_info;
struct variable_symbol_definition;

struct completion_list_instructions
{
    std::string_view completed_text;
    size_t completed_text_start_column;
    const std::unordered_map<const context::macro_definition*, std::shared_ptr<macro_info>>* macros;
    const lsp_context* lsp_ctx;

    std::vector<std::string> additional_instructions;
};

using completion_list_source = std::variant<std::monostate,
    const std::vector<variable_symbol_definition>*,
    const std::unordered_map<context::id_index, context::opencode_sequence_symbol>*,
    const std::unordered_map<context::id_index, context::macro_sequence_symbol>*,
    completion_list_instructions,
    std::pair<const context::macro_definition*, std::vector<std::pair<const context::symbol*, context::id_index>>>>;

} // namespace hlasm_plugin::parser_library::lsp

#endif
