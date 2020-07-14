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

#include "ranged_diagnostic_collector.h"

#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

ranged_diagnostic_collector::ranged_diagnostic_collector(const collectable<diagnostic_s>* diagnoser, range diag_range)
    : s_diagnoser_(diagnoser)
    , op_diagnoser_(nullptr)
    , diag_range_(diag_range)
    , diagnostics_present(false)
{}

ranged_diagnostic_collector::ranged_diagnostic_collector(const collectable<diagnostic_op>* diagnoser, range diag_range)
    : s_diagnoser_(nullptr)
    , op_diagnoser_(diagnoser)
    , diag_range_(diag_range)
    , diagnostics_present(false)
{}

ranged_diagnostic_collector::ranged_diagnostic_collector()
    : s_diagnoser_(nullptr)
    , op_diagnoser_(nullptr)
    , diagnostics_present(false)
{}

void ranged_diagnostic_collector::operator()(const std::function<diagnostic_op(range)>& f)
{
    diagnostics_present = true;
    if (s_diagnoser_)
        s_diagnoser_->add_diagnostic(f(diag_range_));
    else if (op_diagnoser_)
        op_diagnoser_->add_diagnostic(f(diag_range_));
}

} // namespace hlasm_plugin::parser_library
