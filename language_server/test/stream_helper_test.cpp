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

#include <sstream>

#include "gmock/gmock.h"

#include "stream_helper.h"

using namespace hlasm_plugin::language_server;

TEST(stream_helper, stream_helper)
{
    std::stringstream ss(
        "Spaces must not be treated\nas whitespaces and \n\r\n newlines must be \t treated as whitespaces.");
    imbue_stream_newline_is_space(ss);
    std::vector<std::string> lines;
    std::string line;
    while (ss >> line)
    {
        lines.push_back(line);
    }
    std::vector<std::string> expected = {
        "Spaces must not be treated", "as whitespaces and ", "\r", " newlines must be \t treated as whitespaces."
    };
    EXPECT_EQ(lines, expected);
}
