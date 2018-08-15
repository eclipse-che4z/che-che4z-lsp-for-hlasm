#ifndef HLASMPLUGIN_PARSER_HLASMLEX_H
#define HLASMPLUGIN_PARSER_HLASMLEX_H

#include "../generated/parser_library_export.h"
#include "antlr4-runtime.h"
#include <memory>
#include <queue>
#include <set>
#include <map>
#include <string_view>

namespace HlasmPlugin {
	namespace HlasmParserLibrary {

		using token_ptr = std::unique_ptr<antlr4::Token>;
		using char_t = char32_t;
		class PARSER_LIBRARY_EXPORT HlasmLexer : public antlr4::TokenSource
		{
		public:
			HlasmLexer(antlr4::CharStream*);

			HlasmLexer(const HlasmLexer &) = delete;
			HlasmLexer& operator=(const HlasmLexer&) = delete;
			HlasmLexer(HlasmLexer &&) = delete;

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

			bool setBegin(size_t);
			bool setEnd(size_t);
			bool setContinue(size_t);
			void setContinuationEnabled(bool);
			void setICTL();

		protected:
			void createToken(size_t, size_t);
			void consume();

		private:
			std::set<size_t> tokens_after_continueation;
			size_t last_token_id = 0;
			size_t last_continuation = static_cast<size_t>(-1);
			std::queue<token_ptr> token_queue;

			bool DOUBLE_BYTE_ENABLED = false;
			bool CONTINUATION_ENABLED = true;
			bool ICTL = false;
			size_t BEGIN = 0;
			size_t END_DEFAULT = 71;
			size_t END = 71;
			size_t CONTINUE = 15;

			size_t TAB_SIZE = 1;

			std::basic_string<char_t> current_word;

			const Ref<antlr4::TokenFactory<antlr4::CommonToken>> factory
				= antlr4::CommonTokenFactory::DEFAULT;
			antlr4::CharStream* input;

			size_t c;

			size_t line = 1;
			size_t charPositionInLine = 0;

			size_t startCharIndex = 0;
			size_t startLine = 0;
			size_t startCharPositionInLine = 0;

			bool EOF() const;
			bool identifierDivider() const;

			void startToken();
			void lexBegin();
			void lexEnd(bool);
			void lexComment();
			void lexContinuation();
			void lexSpace();
			void lexWord();
			void checkContinuation();
			void lexTokens();
			void consumeNewLine();
			void lexProcess();

			size_t apostrophes = 0;
			bool isProcess() const;
		};
	}
}
#endif
