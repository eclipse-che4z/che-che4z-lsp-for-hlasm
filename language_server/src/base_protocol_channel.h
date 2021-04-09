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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_BASE_PROTOCOL_CHANNEL_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_BASE_PROTOCOL_CHANNEL_H

#include <iosfwd>
#include <mutex>
#include <string>

#include "json_channel.h"

namespace hlasm_plugin::language_server {

// Serializaes and deserializes JSON messages
class base_protocol_channel final : public json_channel
{
    std::mutex write_mutex;

    std::istream& input;
    std::ostream& output;

    std::string message_buffer;

    bool read_message(std::string& out);
    void write_message(const std::string& in);

public:
    // Takes istream to read messages, ostream to write messages
    base_protocol_channel(std::istream& in, std::ostream& out);

    std::optional<nlohmann::json> read() override;
    void write(const nlohmann::json&) override;
    void write(nlohmann::json&&) override;
};

} // namespace hlasm_plugin::language_server

#endif
