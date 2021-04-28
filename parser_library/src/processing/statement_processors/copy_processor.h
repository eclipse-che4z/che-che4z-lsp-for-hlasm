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

#include "context/hlasm_context.h"
#include "copy_processing_info.h"
#include "processing/processing_state_listener.h"
#include "statement_processor.h"

namespace hlasm_plugin::parser_library::processing {

// processor that processes copy members
class copy_processor : public statement_processor
{
    processing_state_listener& listener_;
    copy_start_data start_;

    int macro_nest_;

    copy_processing_result result_;
    bool first_statement_;

public:
    copy_processor(analyzing_context ctx, processing_state_listener& listener, copy_start_data start);

    processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    void collect_diags() const override;

private:
    void process_MACRO();
    void process_MEND();
};

} // namespace hlasm_plugin::parser_library::processing
#endif
