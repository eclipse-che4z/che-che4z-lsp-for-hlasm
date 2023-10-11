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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H
#define HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H

// This file contains definitions of types that the workspace
// manager uses to pass data in and out of parser library.
// Most of them use pimpl to hide their implementation somewhere
// in the library.
// Most of the types are C++ representation of LSP/DAP data types.

#include <cstdint>
#include <cstring>

#include "parser_library_export.h"
#include "range.h"
#include "sequence.h"

#pragma warning(push)
#pragma warning(disable : 4661)

namespace hlasm_plugin::parser_library {

using version_t = uint64_t;

namespace semantics {
struct position_uri_s;
struct completion_list_s;
struct highlighting_info;

// in case any changes are done to these scopes, the tokenTypes field in feature_language_features.cpp
// needs to be adjusted accordingly, as they are implicitly but directly mapped to each other
enum class PARSER_LIBRARY_EXPORT hl_scopes : size_t
{
    label = 0,
    instruction = 1,
    remark = 2,
    ignored = 3,
    comment = 4,
    continuation = 5,
    seq_symbol = 6,
    var_symbol = 7,
    operator_symbol = 8,
    string = 9,
    number = 10,
    operand = 11,
    data_def_type = 12,
    data_def_modifier = 13,
    data_attr_type = 14,
    self_def_type = 15,
    ordinary_symbol = 16
};
} // namespace semantics

namespace debugging {

struct stack_frame;
struct source;
struct scope;
class variable;
struct variable_store;
} // namespace debugging

namespace lsp {
struct completion_item_s;
struct document_symbol_item_s;
} // namespace lsp

struct location;
struct range_uri_s;
class diagnostic_related_info_s;
class diagnostic_s;
struct fade_message_s;

enum class PARSER_LIBRARY_EXPORT completion_trigger_kind
{
    invoked = 1,
    trigger_character = 2,
    trigger_for_incomplete_completions = 3
};

enum class PARSER_LIBRARY_EXPORT completion_item_kind
{
    mach_instr = 0,
    asm_instr = 1,
    ca_instr = 2,
    macro = 3,
    var_sym = 4,
    seq_sym = 5,
    ord_sym = 6,
};

struct PARSER_LIBRARY_EXPORT completion_item
{
    explicit completion_item(const lsp::completion_item_s& item);
    std::string_view label() const;
    completion_item_kind kind() const;
    std::string_view detail() const;
    std::string_view documentation() const;
    std::string_view insert_text() const;
    bool is_snippet() const;
    std::string_view suggestion_for() const;

private:
    const lsp::completion_item_s& item_;
};

using completion_list = sequence<completion_item, const lsp::completion_item_s*>;

enum class PARSER_LIBRARY_EXPORT document_symbol_kind
{
    DAT = 0,
    EQU = 1,
    MACH = 2,
    UNKNOWN = 3,
    VAR = 4,
    SEQ = 5,
    COMMON = 6,
    DUMMY = 7,
    EXECUTABLE = 8,
    READONLY = 9,
    MACRO = 10,
    ASM = 11,
    EXTERNAL = 12,
    WEAK_EXTERNAL = 13
};

struct PARSER_LIBRARY_EXPORT document_symbol_item;
using document_symbol_list = sequence<document_symbol_item, const lsp::document_symbol_item_s*>;

struct PARSER_LIBRARY_EXPORT document_symbol_item
{
    explicit document_symbol_item(const lsp::document_symbol_item_s& item);
    sequence<char> name() const;
    document_symbol_kind kind() const;
    range symbol_range() const;
    range symbol_selection_range() const;
    document_symbol_list children() const;

private:
    const lsp::document_symbol_item_s& item_;
};

struct PARSER_LIBRARY_EXPORT position_uri
{
    explicit position_uri(const location& item);
    position pos() const;
    std::string_view file_uri() const;

private:
    const location& item_;
};

using position_uri_list = sequence<position_uri, const location*>;

struct PARSER_LIBRARY_EXPORT range_uri
{
    explicit range_uri(range_uri_s& range);
    range get_range() const;
    const char* uri() const;

private:
    range_uri_s& impl_;
};

struct PARSER_LIBRARY_EXPORT document_change
{
    document_change(const char* new_text, size_t text_length)
        : whole(true)
        , text(new_text)
        , text_length(text_length)
    {}
    document_change(range change_range, const char* new_text, size_t text_length)
        : whole(false)
        , change_range(change_range)
        , text(new_text)
        , text_length(text_length)
    {}

    bool operator==(const document_change& ch) const
    {
        return whole == ch.whole && change_range == ch.change_range && text_length == ch.text_length
            && std::memcmp(text, ch.text, text_length) == 0;
    }

    const bool whole;
    const range change_range;
    const char* const text;
    const size_t text_length;
};

struct PARSER_LIBRARY_EXPORT text_document_item
{
    char* document_uri;
    version_t version;
    char* text;
};

enum class PARSER_LIBRARY_EXPORT diagnostic_severity
{
    error = 1,
    warning = 2,
    info = 3,
    hint = 4,
    unspecified = 5
};

enum class PARSER_LIBRARY_EXPORT diagnostic_tag
{
    none = 0,
    unnecessary = 1 << 0,
    deprecated = 1 << 1,
};

struct PARSER_LIBRARY_EXPORT diagnostic_related_info
{
    diagnostic_related_info(diagnostic_related_info_s&);

    range_uri location() const;
    const char* message() const;

private:
    diagnostic_related_info_s& impl_;
};

struct PARSER_LIBRARY_EXPORT diagnostic
{
    explicit diagnostic(diagnostic_s&);

    const char* file_uri() const;
    range get_range() const;
    diagnostic_severity severity() const;
    const char* code() const;
    const char* source() const;
    const char* message() const;
    const diagnostic_related_info related_info(size_t index) const;
    size_t related_info_size() const;
    diagnostic_tag tags() const;

private:
    diagnostic_s& impl_;
};

struct PARSER_LIBRARY_EXPORT fade_message
{
    explicit fade_message(fade_message_s&);

    const char* file_uri() const;
    range get_range() const;
    const char* code() const;
    const char* source() const;
    const char* message() const;

private:
    fade_message_s& impl_;
};

struct PARSER_LIBRARY_EXPORT performance_metrics
{
    size_t lines = 0;
    size_t macro_def_statements = 0;
    size_t macro_statements = 0;
    size_t open_code_statements = 0;
    size_t copy_def_statements = 0;
    size_t copy_statements = 0;
    size_t reparsed_statements = 0;
    size_t lookahead_statements = 0;
    size_t continued_statements = 0;
    size_t non_continued_statements = 0;

    bool operator==(const performance_metrics&) const noexcept = default;
};

struct PARSER_LIBRARY_EXPORT workspace_file_info
{
    size_t files_processed = 0;
    bool config_parsing = false;
    bool diagnostics_suppressed = false;
    bool processor_group_found = false;
};

struct PARSER_LIBRARY_EXPORT parsing_metadata
{
    performance_metrics metrics;
    workspace_file_info ws_info;
    size_t errors = 0;
    size_t warnings = 0;
};

struct PARSER_LIBRARY_EXPORT diagnostic_list
{
    diagnostic_list();
    diagnostic_list(diagnostic_s* begin, size_t size);

    diagnostic diagnostics(size_t index);
    size_t diagnostics_size() const;

private:
    diagnostic_s* begin_;
    size_t size_;
};

struct PARSER_LIBRARY_EXPORT fade_message_list
{
    fade_message_list();
    fade_message_list(fade_message_s* begin, size_t size);

    fade_message message(size_t index);
    size_t size() const;

private:
    fade_message_s* begin_;
    size_t size_;
};

struct PARSER_LIBRARY_EXPORT token_info
{
    token_info(const range& token_range, semantics::hl_scopes scope)
        : token_range(token_range)
        , scope(scope)
    {}
    token_info(position start, position end, semantics::hl_scopes scope)
        : token_range(start, end)
        , scope(scope)
    {}
    token_info(size_t line_start, size_t column_start, size_t line_end, size_t column_end, semantics::hl_scopes scope)
        : token_range({ line_start, column_start }, { line_end, column_end })
        , scope(scope)
    {}
    range token_range;
    semantics::hl_scopes scope;

    bool operator==(const token_info& rhs) const noexcept = default;
};

struct PARSER_LIBRARY_EXPORT source
{
    source(const debugging::source& source);

    sequence<char> uri;
};

struct PARSER_LIBRARY_EXPORT stack_frame
{
    explicit stack_frame(const debugging::stack_frame& frame);

    sequence<char> name;
    source source_file;
    range source_range;
    uint32_t id;
};

using stack_frames_t = sequence<stack_frame, const debugging::stack_frame*>;

using frame_id_t = size_t;
using var_reference_t = size_t;

enum class set_type
{
    A_TYPE,
    B_TYPE,
    C_TYPE,
    UNDEF_TYPE
};

struct PARSER_LIBRARY_EXPORT scope
{
    explicit scope(const debugging::scope& impl);

    sequence<char> name;
    var_reference_t variable_reference;
    source source_file;
};

using scopes_t = sequence<scope, const debugging::scope*>;

struct PARSER_LIBRARY_EXPORT variable
{
    explicit variable(const debugging::variable& impl);

    sequence<char> name;
    sequence<char> value;
    var_reference_t variable_reference;
    set_type type;
};

using variables_t = sequence<variable, const debugging::variable_store*>;

struct breakpoint
{
    breakpoint(size_t line)
        : line(line)
    {}
    size_t line;
};

} // namespace hlasm_plugin::parser_library
#pragma warning(pop)
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H
