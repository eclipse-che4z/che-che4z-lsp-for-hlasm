#ifndef SEMANTICS_RANGE_PROVIDER_H
#define SEMANTICS_RANGE_PROVIDER_H

#include "antlr4-runtime.h"
#include "shared/range.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class adjusting_state
{
	NONE, SUBSTITUTION, MACRO_REPARSE
};

struct range_provider
{
public:
	range original_range;
	std::vector<range> original_operand_ranges;
	adjusting_state state;

	range_provider(range original_field_range, adjusting_state state);
	range_provider(range original_field_range, std::vector<range> original_operand_ranges, adjusting_state state);
	range_provider();

	static range union_range(const range& lhs, const range& rhs);

	range get_range(const antlr4::Token* start, const antlr4::Token* stop);
	range get_range(const antlr4::Token* terminal);
	range get_range(antlr4::ParserRuleContext* non_terminal);

	range get_empty_range(const antlr4::Token* start);

	range adjust_range(range r);

private:
	position adjust_position(position pos);
};

}
}
}
#endif
