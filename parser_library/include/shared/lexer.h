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

#ifndef HLASMPLUGIN_PARSER_HLASMLEX_H
#define HLASMPLUGIN_PARSER_HLASMLEX_H

#include "parser_library_export.h"
#include "shared/token_factory.h"
#include "shared/token.h"
#include "antlr4-runtime.h"
#include <memory>
#include <queue>
#include <set>
#include <string_view>
#include "input_source.h"
#include "../src/semantics/lsp_info_processor.h"
#include "range.h"


namespace hlasm_plugin {
	namespace parser_library {
		class input_source;

		using token_ptr = std::unique_ptr<antlr4::Token>;
		using char_t = char32_t;
		class PARSER_LIBRARY_EXPORT lexer : public antlr4::TokenSource
		{
		public:
			struct stream_position { size_t line; size_t offset; };
			lexer(input_source*,semantics::lsp_info_processor * lsp_proc,performance_metrics* metrics = nullptr);

			lexer(const lexer &) = delete;
			lexer& operator=(const lexer&) = delete;
			lexer& operator=(lexer&&) = delete;
			lexer(lexer &&) = delete;

			void reset();
			void append();

			virtual ~lexer() = default;

			token_ptr nextToken() override;

			void delete_token(ssize_t index);

			size_t getLine() const override;

			size_t getCharPositionInLine() override;

			antlr4::CharStream* getInputStream() override;

			std::string getSourceName() override;

			Ref<antlr4::TokenFactory<antlr4::CommonToken>>
				getTokenFactory() override
			{
				return dummy_factory;
			};

			token_factory * get_token_factory()
			{
				return factory_.get();
			}

			bool double_byte_enabled() const;

			void set_double_byte_enabled(bool);

			bool continuation_before_token(size_t token_index);

			enum Tokens {
				#include "../src/grammar/lex.tokens"
			};

			enum Channels {
				DEFAULT_CHANNEL = 0,
				HIDDEN_CHANNEL = 1
			};

			bool set_begin(size_t begin);
			bool set_end(size_t end);
			bool set_continue(size_t cont);
			void set_continuation_enabled(bool);
			void set_ictl();
			void insert_EOLLN();

			void ainsert_front(const std::string &);
			void ainsert_back(const std::string &);
			std::string aread();
			std::unique_ptr<input_source>& get_ainsert_stream();

			static bool ord_char(char_t c);

			bool is_ord_char() const;
			bool is_space() const;
			bool is_data_attribute() const;
			void set_unlimited_line(bool unlimited_lines);
			void set_file_offset(position file_offset);
			void rewind_input(stream_position pos);
			bool is_last_line() const;
			bool eof_generated() const;
			stream_position last_lln_begin_position() const;
			stream_position last_lln_end_position() const;

		protected:
			void create_token(size_t ttype, size_t channel);
			void consume();

		private:
			bool eof_generated_ = false;
			bool last_char_utf16_long_ = false;
			bool creating_var_symbol_ = false;
			void ainsert(const std::string & inp, bool front);
			std::unique_ptr<input_source> ainsert_stream_;
			std::deque<UTF32String> ainsert_buffer_;

			std::set<size_t> tokens_after_continuation_;
			size_t last_token_id_ = 0;
			size_t last_continuation_ = static_cast<size_t>(-1);

			stream_position last_lln_begin_pos_ = { 0,0 };
			stream_position last_lln_end_pos_ = { static_cast<size_t>(-1),static_cast<size_t>(-1) };

			std::queue<token_ptr> token_queue_;
			Ref<antlr4::CommonTokenFactory> dummy_factory;

			bool double_byte_enabled_ = false;
			bool continuation_enabled_ = true;
			bool unlimited_line_ = false;
			bool ictl_ = false;
			size_t begin_ = 0;
			size_t end_default_ = 71;
			size_t end_ = 71;
			size_t continue_ = 15;

			size_t tab_size_ = 1;
			
			std::unique_ptr<token_factory> factory_;
			antlr4::CharStream* input_;
			semantics::lsp_info_processor* lsp_proc_;
			performance_metrics* metrics_;

			struct input_state
			{
				input_source* input = nullptr;
				char_t c = 0;
				size_t line = 0;
				size_t char_position = 0;
				size_t char_position_in_line = 0;
				size_t char_position_in_line_utf16 = 0;
			};

			input_state file_input_state_;
			input_state buffer_input_state_;
			input_state* input_state_ = &file_input_state_;
			bool from_buffer() const;

			input_state token_start_state_;

			size_t apostrophes_ = 0;

			bool eof() const;
			bool identifier_divider() const;

			void start_token();
			void switch_input_streams();
			void lex_begin();
			void lex_end(bool);
			void lex_comment();
			void lex_continuation();
			void lex_space();
			bool before_end() const;
			void lex_word();
			void check_continuation();
			void lex_tokens();
			void consume_new_line();
			void lex_process();
			void set_last_line_pos(size_t idx, size_t line);
			static bool char_start_utf8(unsigned c);
			static size_t length_utf16(const std::string & str);


			bool is_process() const;
		};
	}
}
#endif
