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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_BROKER_H
#define HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_BROKER_H

#include <mutex>

namespace hlasm_plugin::language_server {

// The purpose of this class is to send telemetry messages from dap_servers to lsp_server. That requires
// synchronization, because both servers may be created and destructed in parallel.
class telemetry_broker : public json_sink
{
    json_sink* telemetry_sink = nullptr;
    std::mutex write_mutex;

public:
    // Inherited via json_sink
    void write(const nlohmann::json& msg) override
    {
        std::lock_guard guard(write_mutex);
        if (telemetry_sink)
            telemetry_sink->write(msg);
    }
    void write(nlohmann::json&& msg) override
    {
        std::lock_guard guard(write_mutex);
        if (telemetry_sink)
            telemetry_sink->write(std::move(msg));
    }

    void set_telemetry_sink(json_sink* sink)
    {
        std::lock_guard guard(write_mutex);
        telemetry_sink = sink;
    }
};

} // namespace hlasm_plugin::language_server

#endif
