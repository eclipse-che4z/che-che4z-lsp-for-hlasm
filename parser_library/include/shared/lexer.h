#ifndef HLASMPLUGIN_PARSER_HLASMLEX_H
#define HLASMPLUGIN_PARSER_HLASMLEX_H

#include "../generated/parser_library_export.h"
#include "../shared/token_factory.h"
#include "../shared/token.h"
#include "antlr4-runtime.h"
#include <memory>
#include <queue>
#include <set>
#include <string_view>
#include "input_source.h"

namespace hlasm_plugin {
	namespace parser_library {
		class input_source;

		using token_ptr = std::unique_ptr<antlr4::Token>;
		using char_t = char32_t;
		class PARSER_LIBRARY_EXPORT lexer : public antlr4::TokenSource
		{
		public:
			lexer(input_source*);

			lexer(const lexer &) = delete;
			lexer& operator=(const lexer&) = delete;
			lexer& operator=(lexer&&) = delete;
			lexer(lexer &&) = delete;

			virtual ~lexer() = default;

			token_ptr nextToken() override;

			size_t getLine() const override;

			size_t getCharPositionInLine() override;

			antlr4::CharStream* getInputStream() override;

			std::string getSourceName() override;

			Ref<antlr4::TokenFactory<antlr4::CommonToken>>
				getTokenFactory() override
			{
				return {};
			};

			bool double_byte_enabled() const;

			void set_double_byte_enabled(bool);

			bool continuation_before_token(size_t);

			enum Tokens {
				#include "../src/grammar/lex.tokens"
			};

			enum Channels {
				DEFAULT_CHANNEL = 0,
				HIDDEN_CHANNEL = 1
			};

			bool set_begin(size_t);
			bool set_end(size_t);
			bool set_continue(size_t);
			void set_continuation_enabled(bool);
			void set_ictl();

			void ainsert_front(const std::string &);
			void ainsert_back(const std::string &);
			std::string aread();
			std::unique_ptr<input_source>& get_ainsert_stream();

			bool is_ord_char() const;
			bool get_unlimited_line() const;
			void set_unlimited_line(bool);
			void rewind_input(size_t, size_t);
			bool is_last_line() const;
		protected:
			void create_token(size_t, size_t);
			void consume();

		private:
			bool eof_generated_ = false;
			void ainsert(const std::string &, bool);
			/* UTF8 <-> UTF32 */
			#ifdef __GNUG__
				std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt_;
			#elif _MSC_VER
				std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> cvt_;
			#endif
			std::unique_ptr<input_source> ainsert_stream_;
			std::deque<UTF32String> ainsert_buffer_;

			std::set<size_t> tokens_after_continuation_;
			size_t last_token_id_ = 0;
			size_t last_continuation_ = static_cast<size_t>(-1);

			std::queue<token_ptr> token_queue_;

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

			struct input_state
			{
				input_source* input;
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
			static bool char_start_utf8(unsigned c);
			static size_t length_utf16(const std::string & str);


			bool is_process() const;
		};
	}
}
#endif
