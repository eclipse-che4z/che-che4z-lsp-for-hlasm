#ifndef HLASMPLUGIN_PARSER_HLASMLEX_H
#define HLASMPLUGIN_PARSER_HLASMLEX_H

#include "../generated/parser_library_export.h"
#include "antlr4-runtime.h"
#include <memory>
#include <queue>
#include <set>
#include <map>
#include <string_view>

namespace hlasm_plugin {
	namespace parser_library {

		using token_ptr = std::unique_ptr<antlr4::Token>;
		using char_t = char32_t;
		class PARSER_LIBRARY_EXPORT lexer : public antlr4::TokenSource
		{
		public:
			lexer(antlr4::CharStream*);

			lexer(const lexer &) = delete;
			lexer& operator=(const lexer&) = delete;
			lexer(lexer &&) = delete;

			token_ptr nextToken() override;

			size_t getLine() const override;

			size_t getCharPositionInLine() override;

			antlr4::CharStream* getInputStream() override;

			std::string getSourceName() override;

			Ref<antlr4::TokenFactory<antlr4::CommonToken>>
				getTokenFactory() override;

			bool doubleByteEnabled() const;

			void setDoubleByteEnabled(bool);

			bool continuationBeforeToken(size_t);

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

		protected:
			void create_token(size_t, size_t);
			void consume();

		private:
			std::set<size_t> tokens_after_continuation_;
			size_t last_token_id_ = 0;
			size_t last_continuation_ = static_cast<size_t>(-1);
			std::queue<token_ptr> token_queue_;

			bool double_byte_enabled_ = false;
			bool continuation_enabled_ = true;
			bool ictl_ = false;
			size_t begin_ = 0;
			size_t end_default_ = 71;
			size_t end_ = 71;
			size_t continue_ = 15;

			size_t tab_size_ = 1;

			std::basic_string<char_t> current_word_;

			const Ref<antlr4::TokenFactory<antlr4::CommonToken>> factory_
				= antlr4::CommonTokenFactory::DEFAULT;
			antlr4::CharStream* input_;

			size_t c_;

			size_t line_ = 1;
			size_t char_position_in_line_ = 0;

			size_t start_char_index_ = 0;
			size_t start_line_ = 0;
			size_t start_char_position_in_line_ = 0;

			size_t apostrophes_ = 0;

			bool eof() const;
			bool identifier_divider() const;

			void start_token();
			void lex_begin();
			void lex_end(bool);
			void lex_comment();
			void lex_continuation();
			void lex_space();
			void lex_word();
			void check_continuation();
			void lex_tokens();
			void consume_new_line();
			void lex_process();

			
			bool is_process() const;
		};
	}
}
#endif
