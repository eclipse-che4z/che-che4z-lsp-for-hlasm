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
	return adjust_range(ret);
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
	range ret;
	ret.start.line = start->getLine();
	ret.start.column = start->getCharPositionInLine();
	ret.end = ret.start;
	return adjust_range(ret);
}

range range_provider::adjust_range(range r)
{
	if (state == adjusting_state::MACRO_REPARSE)
	{
		auto offset = r.start.column - original_range.start.column;

		auto column_start = original_range.start.column;
		auto line_start = original_range.start.line;

		while (true)
		{
			auto rest = 71 - column_start;
			if (offset > rest)
			{
				offset -= rest;
				column_start = 15;
				++line_start;
			}
			else
			{
				column_start += offset;
				break;
			}
		}

		offset = r.end.column - r.start.column;
		auto column_end = column_start;
		auto line_end = line_start;

		while (true)
		{
			auto rest = 71 - column_end;
			if (offset > rest)
			{
				offset -= rest;
				column_end = 15;
				++line_end;
			}
			else
			{
				column_end += offset;
				break;
			}
		}

		return range(position(line_start, column_start), position(line_end, column_end));
	}
	else if (state == adjusting_state::SUBSTITUTION)
		return original_range;

	return r;
}

range_provider::range_provider(range original_original_range, adjusting_state state)
	: original_range(original_original_range), state(state) {}

range_provider::range_provider()
	: original_range(), state(adjusting_state::NONE) {}
