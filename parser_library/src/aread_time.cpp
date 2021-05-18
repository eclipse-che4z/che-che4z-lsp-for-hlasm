/*
 * Copyright (c) 2021 Broadcom.
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

#include "aread_time.h"

namespace {
std::string zero_left_pad(std::string s, size_t len)
{
    if (s.size() >= len)
        return s;

    s.insert(s.begin(), len - s.size(), '0');
    return s;
}
} // namespace


std::string hlasm_plugin::parser_library::time_to_clockb(std::chrono::nanoseconds d)
{
    using namespace std::chrono;
    return zero_left_pad(std::to_string(duration_cast<milliseconds>(d).count() / 10), 8);
}

std::string hlasm_plugin::parser_library::time_to_clockd(std::chrono::nanoseconds d)
{
    using namespace std::chrono;
    std::string result;
    result.append(zero_left_pad(std::to_string(duration_cast<hours>(d).count()), 2));
    result.append(zero_left_pad(std::to_string(duration_cast<minutes>(d).count() % 60), 2));
    result.append(zero_left_pad(std::to_string(duration_cast<seconds>(d).count() % 60), 2));
    result.append(zero_left_pad(std::to_string(duration_cast<milliseconds>(d).count() % 1000 / 10), 2));
    return result;
}
