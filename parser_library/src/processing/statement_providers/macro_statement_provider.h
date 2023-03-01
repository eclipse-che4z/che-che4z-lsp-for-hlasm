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

#ifndef PROCESSING_MACRO_STATEMENT_PROVIDER_H
#define PROCESSING_MACRO_STATEMENT_PROVIDER_H

#include "members_statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

// statement provider providing statements of macro definition
class macro_statement_provider final : public members_statement_provider
{
public:
    macro_statement_provider(analyzing_context ctx,
        statement_fields_parser& parser,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener,
        diagnostic_op_consumer& diag_consumer);

    bool finished() const override;

protected:
    std::pair<context::statement_cache*, rollback_token> get_next() override;
    std::vector<diagnostic_op> filter_cached_diagnostics(const semantics::deferred_statement& stmt) const override;
    void go_back(rollback_token t) override;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
