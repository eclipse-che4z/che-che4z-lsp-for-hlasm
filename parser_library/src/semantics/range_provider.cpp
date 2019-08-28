#include "range_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;

range range_provider::union_range(const range& lhs, const range& rhs)
{
	return range(
		position(
			std::min(lhs.start.line, rhs.start.line),
			std::min(lhs.start.column, rhs.start.column)
		),
		position(
			std::max(lhs.end.line, rhs.end.line),
			std::max(lhs.end.column, rhs.end.column)
		)
	);
}

range range_provider::get_range(const antlr4::Token* start, const antlr4::Token* stop)
{
	if (substitute_enabled) return original_range;

	range ret;

	ret.start.line = start->getLine();
	ret.start.column = start->getCharPositionInLine();

	if (stop)
	{
		ret.end.line = stop->getLine();
		ret.end.column = stop->getCharPositionInLine() + stop->getStopIndex() - stop->getStartIndex() + 1;
	}
	else //empty rule
	{
		ret.end = ret.start;
	}

	return ret;
}

range range_provider::get_range(const antlr4::Token* terminal)
{
	return get_range(terminal, terminal);
}

range range_provider::get_range(antlr4::ParserRuleContext* non_terminal)
{
	return get_range(non_terminal->getStart(), non_terminal->getStop());
}

range range_provider::get_empty_range(const antlr4::Token* start)
{
	if (substitute_enabled) return original_range;

	range ret;
	ret.start.line = start->getLine();
	ret.start.column = start->getCharPositionInLine();
	ret.end = ret.start;
	return ret;
}

range_provider::range_provider(range original_original_range, bool enable_substitute)
	: original_range(original_original_range), substitute_enabled(enable_substitute) {}

