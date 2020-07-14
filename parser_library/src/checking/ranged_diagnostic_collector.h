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

#ifndef HLASMPLUGIN_PARSERLIBRARY_RANGED_DIAGNOSTIC_COLLECTOR_H
#define HLASMPLUGIN_PARSERLIBRARY_RANGED_DIAGNOSTIC_COLLECTOR_H

#include <functional>
#include <vector>

#include "diagnosable.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library {

class ranged_diagnostic_collector
{
    const collectable<diagnostic_s>* s_diagnoser_;
    const collectable<diagnostic_op>* op_diagnoser_;
    range diag_range_;

public:
    bool diagnostics_present;

    ranged_diagnostic_collector(const collectable<diagnostic_s>* diagnoser, range diag_range);

    ranged_diagnostic_collector(const collectable<diagnostic_op>* diagnoser, range diag_range);

    ranged_diagnostic_collector();

    void operator()(const std::function<diagnostic_op(range)>& f);
};

} // namespace hlasm_plugin::parser_library
#endif