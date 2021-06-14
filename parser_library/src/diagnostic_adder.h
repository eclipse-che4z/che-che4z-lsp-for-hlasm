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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_ADDER_H
#define HLASMPLUGIN_PARSERLIBRARY_DIAGNOSTIC_ADDER_H

#include <functional>
#include <vector>

#include "diagnosable.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library {

// class that simplyfies adding of diagnostics
// holds both range and collectable object
// hence, there is no need to specify range in a diagnostic creation, just pass diagnostic function

class diagnostic_adder
{
    const std::function<void(diagnostic_op)>* op_diagnoser_ = nullptr;
    range diag_range_;

public:
    bool diagnostics_present;

    diagnostic_adder(const std::function<void(diagnostic_op)>& diagnoser, range diag_range)
        : op_diagnoser_(&diagnoser)
        , diag_range_(diag_range) {};

    diagnostic_adder() {}

    void operator()(const std::function<diagnostic_op(range)>& f)
    {
        if (op_diagnoser_)
            (*op_diagnoser_)(f(diag_range_));
    }
};

} // namespace hlasm_plugin::parser_library

#endif
