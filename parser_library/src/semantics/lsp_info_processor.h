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

#include "../context/hlasm_context.h"
#include <vector>
#include <memory>
#include <regex>

namespace hlasm_plugin {
	namespace parser_library {
		namespace semantics {

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
			completion_list_s() : is_incomplete(true), items() {};
			completion_list_s(bool is_incomplete, std::vector<context::completion_item_s> items) : is_incomplete(is_incomplete), items(items) {};
			bool is_incomplete;
			std::vector<context::completion_item_s> items;
		};

		class lsp_info_processor
		{
		public:
			lsp_info_processor(std::string file, const std::string& text, context::hlasm_context* ctx,bool editing);

			const std::string * file_name;
			const std::string* empty_string;

			void process_hl_symbols(std::vector<token_info> symbols);
			void process_lsp_symbols(std::vector<context::lsp_symbol> symbols, const std::string * given_file = nullptr);

			position_uri_s go_to_definition(const position& pos) const;
			std::vector<position_uri_s> references(const position& pos) const;
			std::vector<std::string> hover(const position& pos) const;
			completion_list_s completion(const position& pos,const char trigger_char, int trigger_kind) const;

			void add_lsp_symbol(context::lsp_symbol& symbol);
			void add_hl_symbol(token_info symbol);

			semantics::highlighting_info& get_hl_info();
		private:
			std::vector<context::var_definition> deferred_vars_;
			context::instr_definition deferred_instruction_;
			std::vector<std::string> text_;
			context::hlasm_context* ctx_;
			semantics::highlighting_info hl_info_;
			bool editing_;
			const std::regex instruction_regex;

			bool is_in_range_(const position& pos, const context::occurence& occ) const;
			template<typename T>
			bool find_definition_(const position& pos, const context::definitions<T>& symbols, position_uri_s& found) const;
			template<typename T>
			bool find_references_(const position& pos, const context::definitions<T>& symbols, std::vector<position_uri_s>& found) const;
			template<typename T>
			bool get_text_(const position& pos, const context::definitions<T>& symbols, std::vector<std::string>& found) const;
			void process_var_syms_();
			void process_seq_sym_(const context::seq_definition& symbol);
			void process_ord_sym_(const context::ord_definition& symbol);
			void process_instruction_sym_();
			completion_list_s complete_var_(const position& pos) const;
			completion_list_s complete_seq_(const position& pos) const;
			int find_latest_version_(const context::instr_definition& current, const context::definitions<context::instr_definition>& to_check) const;
			context::macro_id get_top_macro_stack_();
			};
		}
	}
}
#endif