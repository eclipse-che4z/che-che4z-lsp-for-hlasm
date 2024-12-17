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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSABLE_CTX_H

#include "diagnostic_consumer.h"

namespace hlasm_plugin::parser_library {

namespace context {
class hlasm_context;
} // namespace context

// abstract diagnosable class that enhances collected diagnostics
// adds a stack of nested file positions that indicate where the diagnostic occured
class diagnosable_ctx final : public diagnostic_consumer, public diagnostic_op_consumer
{
    std::vector<diagnostic> collected_diags;
    context::hlasm_context& ctx_;
    size_t limit;

public:
    void add_raw_diagnostic(diagnostic diagnostic);

    void add_diagnostic(diagnostic diagnostic) final;
    void add_diagnostic(diagnostic_op diagnostic) final;

    std::vector<diagnostic>& diags() { return collected_diags; }

    diagnosable_ctx(context::hlasm_context& ctx, size_t limit = static_cast<size_t>(-1))
        : ctx_(ctx)
        , limit(limit)
    {}

    friend class diagnostic_collector;
};

} // namespace hlasm_plugin::parser_library


#endif
