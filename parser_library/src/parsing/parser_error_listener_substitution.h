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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_SUBSTITUTION_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_SUBSTITUTION_H

#include "parser_error_listener.h"
#include "semantics/range_provider.h"

// implementation of parser error listener that provide additional error handling
// used during recursed parsing when nested diagnostic is needed
namespace hlasm_plugin::parser_library::parsing {

class parser_error_listener_substitution : public parser_error_listener_base
{
public:
    parser_error_listener_substitution(const std::function<void(diagnostic_op)>& add_diag,
        const std::string* subst_text,
        semantics::range_provider rng_provider)
        : add_diag_(&add_diag)
        , subst_text_(subst_text)
        , rng_provider_(std::move(rng_provider)) {};

protected:
    void add_parser_diagnostic(
        range diagnostic_range, diagnostic_severity severity, std::string code, std::string message) override
    {
        if (subst_text_)
            message = "While substituting to '" + *subst_text_ + "' => " + message;
        (*add_diag_)(
            diagnostic_op(severity, std::move(code), std::move(message), rng_provider_.adjust_range(diagnostic_range)));
    }

private:
    const std::function<void(diagnostic_op)>* add_diag_;
    const std::string* subst_text_;
    semantics::range_provider rng_provider_;
};

} // namespace hlasm_plugin::parser_library::parsing


#endif
