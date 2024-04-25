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

#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

namespace context {
class hlasm_context;
} // namespace context

// abstract diagnosable class that enhances collected diagnostics
// adds a stack of nested file positions that indicate where the diagnostic occured
class diagnosable_ctx : public diagnosable_impl, public diagnostic_op_consumer
{
    context::hlasm_context& ctx_;

public:
    void add_diagnostic(diagnostic diagnostic) const override;
    void add_diagnostic(diagnostic_op diagnostic) const override;

protected:
    diagnosable_ctx(context::hlasm_context& ctx)
        : ctx_(ctx)
    {}

    virtual ~diagnosable_ctx() = default;

    friend class diagnostic_collector;
};

} // namespace hlasm_plugin::parser_library


#endif
