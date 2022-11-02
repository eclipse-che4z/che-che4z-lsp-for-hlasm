/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_ANALYZER_IF_H
#define HLASMPARSER_PARSERLIBRARY_ANALYZER_IF_H

#include "context/hlasm_context.h"
#include "diagnosable_ctx.h"
#include "semantics/source_info_processor.h"

namespace hlasm_plugin::parser_library {

class analyzer_if : public diagnosable_ctx
{
public:
    analyzer_if(context::hlasm_context& hlasm_ctx)
        : diagnosable_ctx(hlasm_ctx) {};

    virtual const semantics::source_info_processor& source_processor() const = 0;

    virtual void analyze(std::atomic<bool>* cancel = nullptr) = 0;

    virtual const performance_metrics& get_metrics() const = 0;
};

} // namespace hlasm_plugin::parser_library
#endif