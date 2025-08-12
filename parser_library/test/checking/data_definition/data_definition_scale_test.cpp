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

#include "gtest/gtest.h"

#include "checking/data_definition/data_def_types.h"

using namespace hlasm_plugin::parser_library::checking;


TEST(data_def_scale_attribute, P)
{
    data_def_type_P t;

    EXPECT_EQ(t.get_scale_attribute({}, "456.1234,-12,4.587"), 4);
}

TEST(data_def_scale_attribute, P_no_integral)
{
    data_def_type_P t;

    EXPECT_EQ(t.get_scale_attribute({}, ".1234"), 4);
}

TEST(data_def_scale_attribute, P_no_fraction)
{
    data_def_type_P t;

    EXPECT_EQ(t.get_scale_attribute({}, "3."), 0);
}

TEST(data_def_scale_attribute, P_simple_number)
{
    data_def_type_P t;

    EXPECT_EQ(t.get_scale_attribute({}, "3"), 0);
}

TEST(data_def_scale_attribute, H_explicit)
{
    data_def_type_H t;

    EXPECT_EQ(t.get_scale_attribute(5, "3"), 5);
}
