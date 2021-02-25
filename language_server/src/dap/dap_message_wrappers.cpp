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

namespace hlasm_plugin::language_server::dap {

std::optional<nlohmann::json> hlasm_plugin::language_server::dap::message_unwrapper::read()
{
    auto msg = source.read();
    if (msg.has_value() && msg.value().count("params"))
        return std::move(msg.value().at("params"));
    return std::nullopt;
}

void message_wrapper::write(const nlohmann::json& msg)
{
    target.write(nlohmann::json { { "jsonrpc", "2.0" }, { "method", broadcom_tunnel_method }, { "params", msg } });
}

void message_wrapper::write(nlohmann::json&& msg)
{
    target.write(
        nlohmann::json { { "jsonrpc", "2.0" }, { "method", broadcom_tunnel_method }, { "params", std::move(msg) } });
}

} // namespace hlasm_plugin::language_server::dap
