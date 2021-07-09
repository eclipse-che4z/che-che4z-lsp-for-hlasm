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

#ifndef PROCESSING_COPY_STATEMENT_PROVIDER_H
#define PROCESSING_COPY_STATEMENT_PROVIDER_H

#include "members_statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

// statement provider providing statements of copy members
class copy_statement_provider : public members_statement_provider
{
    bool m_suspended = false;

public:
    copy_statement_provider(analyzing_context ctx,
        statement_fields_parser& parser,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener,
        diagnostic_op_consumer& diag_consumer);

    bool finished() const override;

    void suspend();
    bool try_resume_at(size_t line_no, resume_copy resume_opts);

protected:
    context::statement_cache* get_next() override;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
