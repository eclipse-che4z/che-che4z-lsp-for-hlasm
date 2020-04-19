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
#include <iostream>
#define __STDC_WANT_LIB_EXT1__ 1
#include <filesystem>
#include <time.h>

#include "logger.h"

using namespace std;
using namespace hlasm_plugin::language_server;

constexpr const char* log_filename = "hlasmplugin.log";


logger::logger()
{
    auto log_folder = std::filesystem::temp_directory_path();
    auto log_path = log_folder / log_filename;
    file_.open(log_path, ios::out);
}

logger::~logger() { file_.close(); }


void logger::log(const std::string& data) { file_ << current_time() << "  " << data << endl; }

void logger::log(const char* data) { file_ << current_time() << "  " << data << endl; }

string logger::current_time()
{
    string curr_time;
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
    return curr_time.substr(0, curr_time.size() - 1);
}
