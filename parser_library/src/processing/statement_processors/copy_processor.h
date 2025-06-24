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

#ifndef PROCESSING_COPY_PROCESSOR_H
#define PROCESSING_COPY_PROCESSOR_H

#include "copy_processing_info.h"
#include "processing/processing_state_listener.h"
#include "statement_processor.h"

namespace hlasm_plugin::parser_library::processing {

// processor that processes copy members
class copy_processor final : public statement_processor
{
    processing_state_listener& listener_;
    copy_start_data start_;

    int macro_nest_;

    copy_processing_result result_;
    bool first_statement_;

public:
    copy_processor(const analyzing_context& ctx,
        processing_state_listener& listener,
        copy_start_data start,
        diagnosable_ctx& diag_ctx);

    std::optional<processing_status> get_processing_status(
        const std::optional<context::id_index>& instruction, const range& r) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

private:
    void process_MACRO();
    void process_MEND();

    std::optional<context::id_index> resolve_concatenation(
        const semantics::concat_chain& concat, const range& r) const override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
