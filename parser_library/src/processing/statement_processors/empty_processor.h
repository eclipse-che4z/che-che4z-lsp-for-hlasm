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

#ifndef PROCESSING_EMPTY_PROCESSOR_H
#define PROCESSING_EMPTY_PROCESSOR_H

#include "statement_processor.h"

namespace hlasm_plugin::parser_library::processing {

// processor ignoring provided statements
class empty_processor final : public statement_processor
{
public:
    empty_processor(analyzing_context ctx);
    std::optional<processing_status> get_processing_status(const semantics::instruction_si& instruction) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    void collect_diags() const override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
