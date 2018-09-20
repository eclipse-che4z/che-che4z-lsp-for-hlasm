#ifndef HLASMPLUGIN_PARSER_HLASMHTF_H
#define HLASMPLUGIN_PARSER_HLASMHTF_H

#include "../generated/parser_library_export.h"
#include "antlr4-runtime.h"
#include <memory>
#include "token.h"
#include  "TokenFactory.h"

namespace hlasm_plugin {
	namespace parser_library {
		
		class PARSER_LIBRARY_EXPORT token_factory
		{
		public:
			token_factory();

			token_factory(const token_factory &) = delete;
			token_factory& operator=(const token_factory&) = delete;
			token_factory& operator=(token_factory&&) = delete;
			token_factory(token_factory &&) = delete;

			~token_factory();

			std::unique_ptr<token> create(antlr4::TokenSource* source, antlr4::CharStream* stream, size_t type,
			                              size_t channel,
			                              size_t start, size_t stop, size_t line, size_t char_position_in_line,
			                              size_t index,
			                              size_t char_position_in_line_16);
		};

	}
}
#endif
