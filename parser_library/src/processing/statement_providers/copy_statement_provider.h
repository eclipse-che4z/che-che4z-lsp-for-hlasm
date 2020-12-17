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
public:
    copy_statement_provider(context::hlasm_context& hlasm_ctx,
        statement_fields_parser& parser,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener);

    virtual bool finished() const override;

protected:
    virtual context::cached_statement_storage* get_next() override;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
