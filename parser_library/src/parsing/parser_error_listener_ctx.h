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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H

#include "context/hlasm_context.h"
#include "parser_error_listener_substitution.h"

// implementation of parser error listener that provide additional error handling
// used during recursed parsing when nested diagnostic is needed
namespace hlasm_plugin::parser_library::parsing {

class parser_error_listener_ctx : public diagnosable_ctx, public parser_error_listener_substitution
{
public:
    parser_error_listener_ctx(context::hlasm_context& hlasm_ctx,
        const std::string* substituted,
        semantics::range_provider provider = semantics::range_provider())
        : diagnosable_ctx(hlasm_ctx)
        , parser_error_listener_substitution(add_diag_with_ctx, substituted, std::move(provider))
    {}
    void collect_diags() const override {};

private:
    std::function<void(diagnostic_op)> add_diag_with_ctx = [this](diagnostic_op diag) {
        this->add_diagnostic(std::move(diag));
    };
};

} // namespace hlasm_plugin::parser_library::parsing

#endif
