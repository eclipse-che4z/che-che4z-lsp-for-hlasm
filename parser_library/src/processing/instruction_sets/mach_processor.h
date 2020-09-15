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

#ifndef PROCESSING_MACH_PROCESSOR_H
#define PROCESSING_MACH_PROCESSOR_H

#include "low_language_processor.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// processor of machine instructions
class mach_processor : public low_language_processor
{
    checking::machine_checker checker;

public:
    mach_processor(context::hlasm_context& hlasm_ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        statement_fields_parser& parser);

    virtual void process(context::unique_stmt_ptr stmt) override;
    virtual void process(context::shared_stmt_ptr stmt) override;

private:
    void process(rebuilt_statement statement, const op_code& opcode);
};

} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif