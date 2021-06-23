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

#ifndef PROCESSING_PROCESSING_STATE_LISTENER_H
#define PROCESSING_PROCESSING_STATE_LISTENER_H

#include "statement_processors/copy_processing_info.h"
#include "statement_processors/lookahead_processing_info.h"
#include "statement_processors/macrodef_processing_info.h"

namespace hlasm_plugin::parser_library::processing {

enum class resume_copy
{
    ignore_line,
    exact_line_match,
    exact_or_next_line,
};

// interface for listening that a statement processor needs to be started or has finished
class processing_state_listener
{
public:
    virtual void start_macro_definition(macrodef_start_data start) = 0;
    virtual void finish_macro_definition(macrodef_processing_result result) = 0;

    virtual void start_lookahead(lookahead_start_data start) = 0;
    virtual void finish_lookahead(lookahead_processing_result result) = 0;

    virtual void start_copy_member(copy_start_data start) = 0;
    virtual void finish_copy_member(copy_processing_result result) = 0;

    virtual void suspend_opencode_copy_processing() = 0;
    virtual bool resume_opencode_copy_processing_at(size_t line_no, resume_copy resume_opts) = 0;

    virtual void finish_opencode() = 0;

    virtual ~processing_state_listener() = default;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
