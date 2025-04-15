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

#include <type_traits>

#include "diagnostic_consumer.h"

namespace hlasm_plugin::parser_library {

// class that simplyfies adding of diagnostics
// holds both range and collectable object
// hence, there is no need to specify range in a diagnostic creation, just pass diagnostic function

class diagnostic_adder
{
    diagnostic_op_consumer* op_diagnoser_ = nullptr;
    range diag_range_;

public:
    bool diagnostics_present = false;

    diagnostic_adder(diagnostic_op_consumer& diagnoser, range diag_range)
        : op_diagnoser_(&diagnoser)
        , diag_range_(diag_range) {};

    explicit diagnostic_adder(range diag_range)
        : diag_range_(diag_range) {};

    diagnostic_adder() = default;

    template<typename F, typename... Args>
    void operator()(F&& f, Args&&... args) requires std::is_invocable_r_v<diagnostic_op, F, Args..., range>
    {
        diagnostics_present = true;
        if (op_diagnoser_)
            op_diagnoser_->add_diagnostic(f(std::forward<Args>(args)..., diag_range_));
    }
};

} // namespace hlasm_plugin::parser_library

#endif
