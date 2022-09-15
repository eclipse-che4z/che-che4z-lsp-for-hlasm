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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_COLLECTOR_H

#include <vector>

#include "context/source_context.h"
#include "diagnostic.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library {

class diagnosable_ctx;

// functor that takes a diagnostic and stores it in a diagnosable object passed in the constructor
// used to unify diagnostics collecting for checking objects
class diagnostic_collector
{
    const diagnosable_ctx* diagnoser_;
    context::processing_stack_t location_stack_;
    range diag_range_;

public:
    // constructor with explicit location stack
    // used for outputting diagnostic of postponed statements
    diagnostic_collector(const diagnosable_ctx* diagnoser, context::processing_stack_t location_stack);

    // constructor with implicit location stack (acquired from the diagnoser)
    // used for default statement checking
    explicit diagnostic_collector(const diagnosable_ctx* diagnoser);

    // constructor for collector that silences diagnostics
    diagnostic_collector();

    void operator()(diagnostic_op diagnostic) const;

private:
    context::processing_stack_t get_location_stack() const;
};

} // namespace hlasm_plugin::parser_library
#endif