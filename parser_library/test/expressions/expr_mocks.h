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


#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_EXPR_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_EXPR_MOCK_H

inline std::string big_string(char c = '1')
{
    std::string s;
    s.reserve(4000);
    for (size_t i = 0; i < 4000; i++)
        s.push_back(c);
    return s;
}

#endif
