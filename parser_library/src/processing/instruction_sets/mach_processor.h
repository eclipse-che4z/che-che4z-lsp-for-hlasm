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

namespace hlasm_plugin::parser_library::processing {

// processor of machine instructions
class mach_processor : public low_language_processor
{
public:
    mach_processor(analyzing_context ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        statement_fields_parser& parser,
        const processing_manager& proc_mgr);

    void process(std::shared_ptr<const processing::resolved_statement> stmt) override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif