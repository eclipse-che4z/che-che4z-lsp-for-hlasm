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

#include "dispatcher.h"

#include "nlohmann/json.hpp"

#include "logger.h"

namespace hlasm_plugin::language_server {

dispatcher::dispatcher(json_channel_adapter io, server& server, request_manager& req_mngr)
    : channel(io)
    , server_(server)
    , req_mngr_(req_mngr)
{
    server_.set_send_message_provider(this);
}

void dispatcher::reply(const json& message) { channel.write(message); }

int dispatcher::run_server_loop()
{
    int ret = 0;
    for (;;)
    {
        auto message = channel.read();
        if (!message.has_value())
        {
            ret = 1;
            break;
        }

        req_mngr_.add_request(&server_, std::move(message).value());

        // If exit notification came without prior shutdown request, return error 1.
        if (server_.is_exit_notification_received())
        {
            if (server_.is_shutdown_request_received())
                break;
            else
            {
                ret = 1;
                break;
            }
        }
    }

    req_mngr_.finish_server_requests(&server_);
    return ret;
}

} // namespace hlasm_plugin::language_server
