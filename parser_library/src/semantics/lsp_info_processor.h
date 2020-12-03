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

#ifndef LSP_INFO_PROC_INFO
#define LSP_INFO_PROC_INFO

#include <memory>
#include <regex>
#include <vector>

#include "context/hlasm_context.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// representation of position with file uri
struct position_uri_s
{
    position_uri_s() = default;
    position_uri_s(std::string uri, position position)
        : uri(uri)
        , pos(position) {};
    std::string uri;
    position pos;
    bool operator==(const position_uri_s& other) const { return pos == other.pos && uri == other.uri; }
};

// represents list of completion items, used as a repsonse to the lsp request
struct completion_list_s
{
    completion_list_s()
        : is_incomplete(true)
        , items() {};
    completion_list_s(bool is_incomplete, std::vector<context::completion_item_s> items)
        : is_incomplete(is_incomplete)
        , items(items) {};
    bool is_incomplete;
    std::vector<context::completion_item_s> items;
};

// lsp info processor processes lsp symbols from parser into symbol definitions and their occurencies used for responses
// to lsp requests
class lsp_info_processor
{
public:
    lsp_info_processor(std::string file, const std::string& text, context::hlasm_context* ctx, bool collect_hl_info);

    // name of file this processor is currently used
    const std::string* file_name;
    // common value of an empty string
    const std::string* empty_string;

    // takes vector of highlighting symbols and processes them into highlighting info for further propagation
    void process_hl_symbols(std::vector<token_info> symbols);
    // processes vector of lsp symbols and stores them in the lsp context
    void process_lsp_symbols(std::vector<context::lsp_symbol> symbols, const std::string* given_file = nullptr);

    // handling of implemented lsp requests
    position_uri_s go_to_definition(const position& pos) const;
    std::vector<position_uri_s> references(const position& pos) const;
    std::vector<std::string> hover(const position& pos) const;
    completion_list_s completion(const position& pos, const char trigger_char, int trigger_kind) const;
    const lines_info& semantic_tokens() const;

    // add one lsp symbol to the context
    void add_lsp_symbol(context::lsp_symbol& symbol);
    // add one hl symbol to the highlighting info
    void add_hl_symbol(token_info symbol);

    // finishes collected data
    void finish();
private:
    // stored symbols that couldn't be processed without further information
    std::vector<context::var_definition> deferred_vars_;
    context::instr_definition deferred_instruction_;
    // text of the processed file
    std::vector<std::string> text_;
    // pointer to the hlasm context to retrieve additional information
    context::hlasm_context* ctx_;
    // highlighting information
    semantics::highlighting_info hl_info_;
    // specifies whether to generate highlighting information
    bool collect_hl_info_;
    // regex that represents a common position of instruction within a statement
    const std::regex instruction_regex;

    // checks whether the given position is within occurence's range
    bool is_in_range_(const position& pos, const context::occurence& occ) const;
    // within a given set of symbols, checks whether it contains a symbol on a given position and returns its
    // definition's position
    template<typename T>
    bool find_definition_(const position& pos, const context::definitions<T>& symbols, position_uri_s& found) const;
    // within a given set of symbols, checks whether it contains a symbol on a given position and returns all of its
    // occurences
    template<typename T>
    bool find_references_(
        const position& pos, const context::definitions<T>& symbols, std::vector<position_uri_s>& found) const;
    // within a given set of symbols, checks whether it contains a symbol on a given position and returns its contents
    template<typename T>
    bool get_text_(const position& pos, const context::definitions<T>& symbols, std::vector<std::string>& found) const;
    // processes deferred variable symbols
    void process_var_syms_();
    // processes current sequence symbol
    void process_seq_sym_(const context::seq_definition& symbol);
    // processes current ordinary symbol
    void process_ord_sym_(const context::ord_definition& symbol);
    // processes deferred instruction symbol
    void process_instruction_sym_();
    // responds to completion request on variable symbol
    completion_list_s complete_var_(const position& pos) const;
    // responds to completion request on sequence symbols
    completion_list_s complete_seq_(const position& pos) const;
    // finds the latest version of macro
    int find_latest_version_(const context::instr_definition& current,
        const context::definitions<context::instr_definition>& to_check) const;
    // returns the first macro id on the parsing stack
    context::macro_id get_top_macro_stack_() const;
};
} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif