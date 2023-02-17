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

#include "debug_event_consumer_s_mock.h"

#include <chrono>
#include <thread>

using namespace hlasm_plugin::parser_library;
using namespace std::chrono_literals;

void debug_event_consumer_s_mock::stopped(
    sequence<char> reason, hlasm_plugin::parser_library::sequence<char> addtl_info)
{
    (void)reason;
    (void)addtl_info;
    ++stop_count;
    stopped_ = true;
}

void debug_event_consumer_s_mock::wait_for_stopped()
{
    while (!stopped_)
        d.analysis_step();
    stopped_ = false;
}

void debug_event_consumer_s_mock::wait_for_exited()
{
    while (!exited_)
        d.analysis_step();
    stopped_ = false;
}

void debug_event_consumer_s_mock::exited(int exit_code)
{
    (void)exit_code;
    exited_ = true;
}
