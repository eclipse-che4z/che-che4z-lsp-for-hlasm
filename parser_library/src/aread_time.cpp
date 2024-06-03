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

#include <format>

std::string hlasm_plugin::parser_library::time_to_clockb(std::chrono::nanoseconds d)
{
    using namespace std::chrono;
    return std::format("{:08}", duration_cast<milliseconds>(d).count() / 10);
}

std::string hlasm_plugin::parser_library::time_to_clockd(std::chrono::nanoseconds d)
{
    using namespace std::chrono;
    return std::format("{:02}{:02}{:02}{:02}",
        duration_cast<hours>(d).count(),
        duration_cast<minutes>(d).count() % 60,
        duration_cast<seconds>(d).count() % 60,
        duration_cast<milliseconds>(d).count() % 1000 / 10);
}
