/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LSP_ITEM_CONVERTORS_H
#define HLASMPLUGIN_PARSERLIBRARY_LSP_ITEM_CONVERTORS_H

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "completion_list_source.h"

namespace hlasm_plugin::parser_library {
struct completion_item;

namespace context {
class macro_definition;
class section;
struct sequence_symbol;
class symbol;
struct using_context_description;
} // namespace context
} // namespace hlasm_plugin::parser_library


namespace hlasm_plugin::parser_library::lsp {
class file_info;
struct macro_info;
class text_data_view;
struct variable_symbol_definition;

std::string hover_text(const context::symbol& sym);
std::string hover_text(const variable_symbol_definition& sym);
std::string hover_text(std::span<const context::using_context_description> usings);
void append_hover_text(std::string& buffer, const context::using_context_description& u);
std::string get_macro_documentation(const text_data_view& macro_text, size_t definition_line);
std::string get_logical_line(const text_data_view& text, size_t definition_line);
std::string get_macro_signature(const context::macro_definition& m);
bool is_continued_line(std::string_view line);

completion_item generate_completion_item(const context::sequence_symbol& sym);
completion_item generate_completion_item(const variable_symbol_definition& sym);
completion_item generate_completion_item(const macro_info& sym, const file_info* info);

std::vector<completion_item> generate_completion(const completion_list_source& cls);
std::vector<completion_item> generate_completion(std::monostate);
std::vector<completion_item> generate_completion(const std::vector<variable_symbol_definition>*);
std::vector<completion_item> generate_completion(
    const std::unordered_map<context::id_index, std::unique_ptr<context::sequence_symbol>>*);
std::vector<completion_item> generate_completion(const completion_list_instructions&);
std::vector<completion_item> generate_completion(const std::pair<const context::macro_definition*,
    std::vector<std::pair<const context::symbol*, context::id_index>>>&);

} // namespace hlasm_plugin::parser_library::lsp

#endif
