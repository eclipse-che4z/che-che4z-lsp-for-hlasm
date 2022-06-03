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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_LSP_CONTEXT_TEST_HELPER_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_LSP_CONTEXT_TEST_HELPER_H

#include "gtest/gtest.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::utils::resource;

inline void check_location_with_position(const location& result,
    const resource_location& expected_resource_location,
    size_t expected_line,
    size_t expected_column)
{
    EXPECT_EQ(result, location(position(expected_line, expected_column), expected_resource_location));
}

#endif