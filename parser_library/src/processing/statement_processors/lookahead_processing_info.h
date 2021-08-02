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

#ifndef PROCESSING_LOOKAHEAD_PROCESSING_INFO_H
#define PROCESSING_LOOKAHEAD_PROCESSING_INFO_H

#include "context/ordinary_assembly/symbol.h"
#include "context/source_snapshot.h"

namespace hlasm_plugin::parser_library::processing {

enum class lookahead_action
{
    SEQ,
    ORD
};

// data to start lookahead_processor
struct lookahead_start_data
{
    lookahead_action action;
    context::source_position statement_position;
    context::source_snapshot snapshot;

    // SEQ action
    context::id_index target;
    range target_range;

    lookahead_start_data(context::id_index target,
        range target_range,
        context::source_position statement_position,
        context::source_snapshot snapshot)
        : action(lookahead_action::SEQ)
        , statement_position(statement_position)
        , snapshot(std::move(snapshot))
        , target(target)
        , target_range(target_range)
    {}

    // ORD action
    context::id_set targets;

    lookahead_start_data(
        context::id_set targets, context::source_position statement_position, context::source_snapshot snapshot)
        : action(lookahead_action::ORD)
        , statement_position(statement_position)
        , snapshot(std::move(snapshot))
        , target(nullptr)
        , targets(std::move(targets))
    {}
};

// result of lookahead_processor
struct lookahead_processing_result
{
    lookahead_action action;
    bool success;

    context::source_position statement_position;
    context::source_snapshot snapshot;

    context::id_index symbol_name;
    range symbol_range;

    lookahead_processing_result(lookahead_start_data&& initial_data)
        : action(initial_data.action)
        , success(false)
        , statement_position(initial_data.statement_position)
        , snapshot(std::move(initial_data.snapshot))
        , symbol_name(initial_data.target)
        , symbol_range(initial_data.target_range)
    {}

    lookahead_processing_result(context::id_index target, range target_range)
        : action(lookahead_action::SEQ)
        , success(true)
        , symbol_name(target)
        , symbol_range(target_range)
    {}
};

} // namespace hlasm_plugin::parser_library::processing

#endif
