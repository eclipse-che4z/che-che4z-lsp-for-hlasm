#ifndef HLASMPLUGIN_PARSER_HLASMTOKEN_H
#define HLASMPLUGIN_PARSER_HLASMTOKEN_H

#include "../generated/parser_library_export.h"
#include "antlr4-runtime.h"
#include "input_source.h"
#include <string>

namespace hlasm_plugin {
	namespace parser_library {

		class PARSER_LIBRARY_EXPORT token : public antlr4::Token {
		public:
			token(antlr4::TokenSource *, antlr4::CharStream *, size_t,
				size_t, size_t, size_t, size_t, size_t, size_t, size_t);

			std::string getText() const override;

			size_t getType() const override;

			size_t getLine() const override;

			size_t getCharPositionInLine() const override;

			size_t getChannel() const override;

			size_t getTokenIndex() const override;

			size_t getStartIndex() const override;

			size_t getStopIndex() const override;

			antlr4::TokenSource *getTokenSource() const override;

			antlr4::CharStream *getInputStream() const override;

			std::string toString() const override;

			size_t get_char_position_in_line_16() const;

		private:
			antlr4::TokenSource * source_{};
			antlr4::CharStream * input_{};
			size_t type_;
			size_t channel_;
			size_t start_;
			size_t stop_;
			size_t line_;
			size_t char_position_in_line_;
			size_t token_index_;
			size_t char_position_in_line_16_;
		};
	}
}
#endif
