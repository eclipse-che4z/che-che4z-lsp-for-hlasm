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

#include "../context/lsp_context.h"
#include <vector>
#include <memory>
#include <regex>

namespace hlasm_plugin {
	namespace parser_library {
		namespace semantics {

		enum class symbol_type {seq, var, ord, instruction, hl };

		struct position_uri_s {
			position_uri_s() = default;
			position_uri_s(std::string uri, position position) : uri(uri), pos(position) {};
			std::string uri;
			position pos;
			bool operator==(const position_uri_s& other) const
			{
				return pos == other.pos && uri == other.uri;
			}
		};

		struct completion_list_s 
		{
			bool is_incomplete;
			std::vector<context::completion_item_s> items;
		};
		
		struct lsp_symbol
		{
			lsp_symbol(std::string name, range symbol_range, symbol_type type) : name(std::move(name)), symbol_range(std::move(symbol_range)), type(type), value("") {}
			std::string name;
			file_range symbol_range;
			symbol_type type;
			std::string value;
		};

		class lsp_info_processor
		{
		public:
			lsp_info_processor(std::string file, const std::string& text, context::lsp_ctx_ptr ctx);

			std::string file_name;

			void process_hl_symbols(std::vector<token_info> symbols);
			void process_lsp_symbols(std::vector<lsp_symbol> symbols, const std::string& given_file = "");

			position_uri_s go_to_definition(const position& pos) const;
			std::vector<position_uri_s> references(const position& pos) const;
			std::vector<std::string> hover(const position& pos) const;
			completion_list_s completion(const position& pos,const char trigger_char, int trigger_kind) const;

			void add_lsp_symbol(lsp_symbol symbol);
			void add_hl_symbol(token_info symbol);

			semantics::highlighting_info& get_hl_info();
		private:
			std::vector<context::definition> deferred_vars_;
			context::definition deferred_instruction_;
			std::vector<std::string> text_;
			context::lsp_ctx_ptr ctx_;
			semantics::highlighting_info hl_info_;
			const std::regex instruction_regex;

			bool is_in_range_(const position& pos, const context::occurence& occ) const;
			bool find_definition_(const position& pos, const context::definitions& symbols, position_uri_s& found) const;
			bool find_references_(const position& pos, const context::definitions& symbols, std::vector<position_uri_s>& found) const;
			bool get_text_(const position& pos, const context::definitions& symbols, std::vector<std::string>& found) const;
			void process_var_syms_();
			void process_seq_sym_(context::definition & symbol);
			void process_ord_sym_(context::definition & symbol);
			void process_instruction_sym_();
			completion_list_s complete_var_(const position& pos) const;
			completion_list_s complete_seq_(const position& pos) const;
			context::definition find_latest_version_(const context::definition& current, const context::definitions& to_check) const;

			std::vector<std::string> split_(const std::string& text);
			std::string implode_(const std::vector<std::string>::const_iterator& lines_begin, const std::vector<std::string>::const_iterator& lines_end) const;
		};
		}
	}
}
#endif