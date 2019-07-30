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
			std::string name;
			range symbol_range;
			symbol_type type;
		};

		class lsp_info_processor
		{
		public:
			lsp_info_processor(std::string file, const std::string& text, context::lsp_ctx_ptr ctx);

			std::string file_name;

			void process_hl_symbols(std::vector<token_info> symbols);
			void process_lsp_symbols(std::vector<lsp_symbol> symbols);

			position_uri_s go_to_definition(const position& pos) const;
			std::vector<position_uri_s> references(const position& pos) const;
			std::vector<std::string> hover(const position& pos) const;
			completion_list_s completion(const position& pos,const char trigger_char, int trigger_kind) const;

			void add_lsp_symbol(lsp_symbol symbol);
			void add_hl_symbol(token_info symbol);

			semantics::highlighting_info& get_hl_info();
		private:
			std::vector<context::definition> deferred_seqs_;
			std::vector<context::definition> deferred_vars_;
			context::definition deferred_instruction_;
			std::stack<std::string> parser_macro_stack_;
			const std::string& text_;
			context::lsp_ctx_ptr ctx_;
			semantics::highlighting_info hl_info_;
			const std::regex instruction_regex;

			bool is_in_range_(const position& pos, const context::occurence& occ) const;
			bool find_definition_(const position& pos, const context::definitions& symbols, position_uri_s& found) const;
			bool find_references_(const position& pos, const context::definitions& symbols, std::vector<position_uri_s>& found) const;
			bool get_text_(const position& pos, const context::definitions& symbols, std::vector<std::string>& found) const;
			std::string get_line_(position_t line_no, size_t &start) const;
			void process_ord_sym_(const context::definition & symbol);
			void process_var_syms_();
			void process_seq_sym_(context::definition & symbol);
			void process_instruction_sym_();
			completion_list_s complete_var_(const position& pos) const;
			completion_list_s complete_seq_(const position& pos) const;
		};
		}
	}
}
#endif