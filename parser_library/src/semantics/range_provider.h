#ifndef SEMANTICS_RANGE_PROVIDER_H
#define SEMANTICS_RANGE_PROVIDER_H

#include "antlr4-runtime.h"
#include "shared/range.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

struct range_provider
{
public:
	range original_range;
	bool substitute_enabled;

	range_provider(range original_field_range, bool enable_substitute = false);

	static range union_range(const range& lhs, const range& rhs);

	range get_range(const antlr4::Token* start, const antlr4::Token* stop);
	range get_range(const antlr4::Token* terminal);
	range get_range(antlr4::ParserRuleContext* non_terminal);

	range get_empty_range(const antlr4::Token* start);
};

}
}
}
#endif
