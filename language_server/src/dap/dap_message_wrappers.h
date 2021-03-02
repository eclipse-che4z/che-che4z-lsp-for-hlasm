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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_DAP_DAP_MESSAGE_WRAPPERS_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_DAP_DAP_MESSAGE_WRAPPERS_H

#include "../json_channel.h"

namespace hlasm_plugin::language_server::dap {

constexpr const std::string_view broadcom_tunnel_method = "broadcom/hlasm/dsp_tunnel";

class message_unwrapper final : public json_source
{
    json_source& source;

public:
    message_unwrapper(json_source& s)
        : source(s)
    {}
    std::optional<nlohmann::json> read() override;
};

class message_wrapper final : public json_sink
{
    json_sink& target;
    size_t session_id;

public:
    message_wrapper(json_sink& t, size_t s_id)
        : target(t)
        , session_id(s_id)
    {}
    void write(const nlohmann::json& msg) override;
    void write(nlohmann::json&& msg) override;

    static std::string generate_method_name(size_t id);
};
} // namespace hlasm_plugin::language_server::dap

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_DAP_DAP_MESSAGE_WRAPPERS_H
