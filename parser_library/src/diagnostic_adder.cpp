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

#include "diagnostic_adder.h"

#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

diagnostic_adder::diagnostic_adder(const collectable<diagnostic_s>* diagnoser, range diag_range)
    : s_diagnoser_(diagnoser)
    , op_diagnoser_(nullptr)
    , diag_range_(diag_range)
    , diagnostics_present(false)
{}

diagnostic_adder::diagnostic_adder(const collectable<diagnostic_op>* diagnoser, range diag_range)
    : s_diagnoser_(nullptr)
    , op_diagnoser_(diagnoser)
    , diag_range_(diag_range)
    , diagnostics_present(false)
{}

diagnostic_adder::diagnostic_adder()
    : s_diagnoser_(nullptr)
    , op_diagnoser_(nullptr)
    , diagnostics_present(false)
{}

void diagnostic_adder::operator()(const std::function<diagnostic_op(range)>& f)
{
    diagnostics_present = true;
    if (s_diagnoser_)
        s_diagnoser_->add_diagnostic(f(diag_range_));
    else if (op_diagnoser_)
        op_diagnoser_->add_diagnostic(f(diag_range_));
}

} // namespace hlasm_plugin::parser_library
