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

namespace hlasm_plugin::parser_library::semantics {

// state of range adjusting
enum class adjusting_state
{
    NONE,
    SUBSTITUTION,
    MACRO_REPARSE,
    MODEL_REPARSE,
};

// structure for computing range
struct range_provider
{
public:
    range original_range;
    std::vector<range> original_operand_ranges;
    std::vector<std::pair<std::pair<size_t, bool>, range>> model_substitutions;
    adjusting_state state;

    explicit range_provider(range original_field_range, adjusting_state state);
    explicit range_provider(
        range original_field_range, std::vector<range> original_operand_ranges, adjusting_state state);
    explicit range_provider(std::vector<std::pair<std::pair<size_t, bool>, range>> model_substitutions);
    explicit range_provider();

    range get_range(const antlr4::Token* start, const antlr4::Token* stop) const;
    range get_range(const antlr4::Token* terminal) const;
    range get_range(antlr4::ParserRuleContext* non_terminal) const;

    range get_empty_range(const antlr4::Token* start) const;

    range adjust_range(range r) const;

private:
    position adjust_position(position pos, bool end) const;
    position adjust_model_position(position pos, bool end) const;
};

} // namespace hlasm_plugin::parser_library::semantics
#endif
