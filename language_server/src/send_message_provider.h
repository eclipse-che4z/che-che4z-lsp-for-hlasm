/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_SEND_MESSAGE_PROVIDER_H
#define HLASMPLUGIN_LANGUAGESERVER_SEND_MESSAGE_PROVIDER_H

#include "nlohmann/json_fwd.hpp"

namespace hlasm_plugin::language_server {

// Interface that the server uses to send messages to the LSP client.
class send_message_provider
{
protected:
    ~send_message_provider() = default;

public:
    // Serializes the json and sends it to the LSP client.
    virtual void reply(const nlohmann::json& result) = 0;
};

} // namespace hlasm_plugin::language_server

#endif
