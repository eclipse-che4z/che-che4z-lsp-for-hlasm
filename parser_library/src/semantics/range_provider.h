/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef SEMANTICS_RANGE_PROVIDER_H
#define SEMANTICS_RANGE_PROVIDER_H

#include "antlr4-runtime.h"
#include "range.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

//state of range adjusting
enum class adjusting_state
{
	NONE, SUBSTITUTION, MACRO_REPARSE
};

//structure for computing range
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
