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

#include <iostream>
#include <string>

#include "utils/platform.h"
#include "utils/time.h"

using namespace hlasm_plugin::language_server;

std::string current_time()
{
    auto t = hlasm_plugin::utils::timestamp::now();
    if (!t.has_value())
        return "<unknown time>";
    else
        return t->to_string();
}

void logger::log(std::string_view data)
{
    std::lock_guard g(mutex_);

    utils::platform::log(current_time(), " ", data);
}
