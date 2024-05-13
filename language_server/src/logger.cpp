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

#include "logger.h"

#include <string>
#include <string_view>

#include "utils/platform.h"
#include "utils/time.h"

namespace hlasm_plugin::language_server {

logger logger::instance;

std::string current_time()
{
    auto t = hlasm_plugin::utils::timestamp::now();
    if (!t.has_value())
        return "<unknown time>";
    else
        return t->to_string();
}

void logger::log_impl(unsigned level, std::span<std::string_view> args)
{
    static constexpr std::string_view levels[] = {
        "ERROR",
        "WARN",
        "INFO",
    };
    if (level >= std::size(levels))
        level = std::size(levels) - 1;

    std::lock_guard g(m_mutex);

    auto t = current_time();

    args[0] = t;
    args[1] = ":";
    args[2] = levels[level];
    args[3] = ":";

    utils::platform::log(args);
}

} // namespace hlasm_plugin::language_server
