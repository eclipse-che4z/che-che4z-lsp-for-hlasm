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


#include <cstdlib>
#define __STDC_WANT_LIB_EXT1__ 1
#include <iostream>
#include <time.h>

#include "logger.h"

using namespace hlasm_plugin::language_server;

std::string current_time()
{
    std::string curr_time;
    // Current date/time based on current time
    time_t now = time(0);
    // Convert current time to string

#if __STDC_LIB_EXT1__ || _MSVC_LANG
    char cstr[50];
    ctime_s(cstr, sizeof cstr, &now);
    curr_time.assign(cstr);
#else
    curr_time.assign(ctime(&now));
#endif

    // Last charactor of currentTime is "\n", so remove it
    if (!curr_time.empty())
        curr_time.pop_back();

    return curr_time;
}

void logger::log(std::string_view data)
{
    std::lock_guard g(mutex_);

    std::clog << current_time() << " " << data << '\n';
}
