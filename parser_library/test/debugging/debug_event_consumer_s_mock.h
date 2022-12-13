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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H

#include <atomic>

#include "debugger.h"

class debug_event_consumer_s_mock : public hlasm_plugin::parser_library::debugging::debug_event_consumer
{
    std::atomic<bool> stopped_ = false;
    std::atomic<bool> exited_ = false;
    std::atomic<size_t> stop_count = 0;

public:
    void stopped(hlasm_plugin::parser_library::sequence<char> reason,
        hlasm_plugin::parser_library::sequence<char> addtl_info) override;

    void exited(int exit_code) override;


    void wait_for_stopped();

    void wait_for_exited();
};

#endif // !HLASMPLUGIN_PARSERLIBRARY_TEST_DEBUG_EVENT_CONSUMER_S_MOCK_H
