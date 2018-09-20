#include "../include/shared/token_factory.h"

#include <assert.h> 

using namespace hlasm_plugin;
using namespace parser_library;

token_factory::token_factory()
= default;

token_factory::~token_factory()
= default;

std::unique_ptr<token> token_factory::create(antlr4::TokenSource * source, antlr4::CharStream * stream, size_t type, size_t channel, size_t start, size_t stop, size_t line, size_t char_position_in_line, size_t index, size_t char_position_in_line_16)
{
	return std::make_unique<token>(
		source,
		stream,
		type,
		channel,
		start,
		stop,
		line,
		char_position_in_line,
		index,
		char_position_in_line_16);
}
