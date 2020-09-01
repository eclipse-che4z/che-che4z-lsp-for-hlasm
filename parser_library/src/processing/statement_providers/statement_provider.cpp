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

#include "statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

statement_provider::statement_provider(const statement_provider_kind kind)
    : kind(kind)
{}

bool statement_provider::try_trigger_attribute_lookahead(const semantics::instruction_si& instruction,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener)
{
    if (instruction.type != semantics::instruction_si_type::CONC)
        return false;

    const auto& chain = std::get<semantics::concat_chain>(instruction.value);

    auto references = semantics::concatenation_point::get_undefined_attributed_symbols(chain, eval_ctx);

    if (references.empty())
        return false;

    context::source_position statement_position(
        (size_t)eval_ctx.hlasm_ctx.current_source().end_line, eval_ctx.hlasm_ctx.current_source().begin_index);

    context::source_snapshot snapshot = eval_ctx.hlasm_ctx.current_source().create_snapshot();

    listener.start_lookahead(lookahead_start_data(std::move(references), statement_position, std::move(snapshot)));

    return true;

    /*
    if (attr_lookahead_stop_ && hlasm_ctx_.current_source().end_index < attr_lookahead_stop_.end_index)
        perform_opencode_jump(
            context::source_position(attr_lookahead_stop_.end_line + 1, attr_lookahead_stop_.end_index),
            attr_lookahead_stop_);

    while (true)
    {
        // macro statement provider is not relevant in attribute lookahead
        // provs_.size() is always more than 2, it results from calling constructor
        auto& opencode_prov = **(provs_.end() - 1);
        auto& copy_prov = **(provs_.end() - 2);
        auto& prov = !copy_prov.finished() ? copy_prov : opencode_prov;

        if (prov.finished() || proc.finished())
            break;

        prov.process_next(proc);
    }

    attr_lookahead_stop_ = hlasm_ctx_.current_source().create_snapshot();

    perform_opencode_jump(statement_position, std::move(snapshot));

    auto ret = proc.collect_found_refereces();

    for (auto& sym : ret)
        resolved_symbols.insert(std::move(sym));

    return resolved_symbols;*/
}

bool statement_provider::try_trigger_attribute_lookahead(const context::hlasm_statement& statement,
    expressions::evaluation_context eval_ctx,
    processing::processing_state_listener& listener)
{
    return false;
}

} // namespace hlasm_plugin::parser_library::processing
