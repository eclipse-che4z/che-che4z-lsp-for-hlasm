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

#include "lib_config.h"

namespace hlasm_plugin::parser_library {

const lib_config default_config {
    .diag_supress_limit = 10,
};

namespace {
lib_config combine_two_configs(lib_config combined, const lib_config& second)
{
    if (!combined.diag_supress_limit.has_value())
        combined.diag_supress_limit = second.diag_supress_limit;
    return combined;
}
} // namespace

lib_config lib_config::fill_missing_settings(const lib_config& second) const
{
    return combine_two_configs(combine_two_configs(*this, second), default_config);
}

} // namespace hlasm_plugin::parser_library
