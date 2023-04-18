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

#include "dap_message_wrappers.h"

#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server::dap {

std::optional<nlohmann::json> message_unwrapper::read()
{
    if (auto msg = source.read(); msg.has_value())
    {
        if (msg->is_discarded())
            return msg;
        auto it = msg.value().find("params");
        if (it != msg.value().end() && !it->is_null())
            return std::move(*it);
    }
    return std::nullopt;
}

void message_wrapper::write(const nlohmann::json& msg)
{
    target.write(
        nlohmann::json { { "jsonrpc", "2.0" }, { "method", generate_method_name(session_id) }, { "params", msg } });
}

void message_wrapper::write(nlohmann::json&& msg)
{
    target.write(nlohmann::json {
        { "jsonrpc", "2.0" }, { "method", generate_method_name(session_id) }, { "params", std::move(msg) } });
}

std::string message_wrapper::generate_method_name(size_t id)
{
    return std::string(broadcom_tunnel_method) + '/' + std::to_string(id);
}
} // namespace hlasm_plugin::language_server::dap
