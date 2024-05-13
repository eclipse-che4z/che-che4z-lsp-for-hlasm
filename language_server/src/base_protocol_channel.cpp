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

#include "base_protocol_channel.h"

#include <charconv>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "logger.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server {

base_protocol_channel::base_protocol_channel(std::istream& in, std::ostream& out)
    : input(in)
    , output(out)
{}

constexpr const std::string_view content_length_string = "Content-Length: ";
constexpr const size_t message_size_limit = 1 << 30;
constexpr const std::string_view lsp_header_end = "\r\n\r\n";

void base_protocol_channel::write_message(const std::string& in)
{
    LOG_INFO(in);
    std::lock_guard guard(write_mutex);
    if (!output.good())
    {
        LOG_INFO("Output error.");
        return;
    }
    output.write(content_length_string.data(), content_length_string.size());
    std::string size = std::to_string(in.size());
    output.write(size.c_str(), size.size());
    output.write(lsp_header_end.data(), lsp_header_end.size());
    output.write(in.c_str(), in.size());
    output.flush();
}

void base_protocol_channel::write(const nlohmann::json& message) { write_message(message.dump()); }

void base_protocol_channel::write(nlohmann::json&& message) { write_message(message.dump()); }

bool base_protocol_channel::read_message(std::string& out)
{
    // A Language Server Protocol message starts with a set of HTTP headers,
    // delimited  by \r\n, and terminated by an empty line (\r\n).
    std::size_t content_length = 0;
    std::string line;
    for (;;)
    {
        if (input.eof() || input.fail())
            return false;
        // Reads characters until the next newline '\n'
        input >> line;
        std::string_view line_view = line;

        // Content-Length is a mandatory header, and the only one we handle.
        if (line_view.substr(0, content_length_string.size()) == content_length_string)
        {
            if (content_length != 0)
            {
                LOG_WARNING("Duplicate Content-Length header received. The first one is ignored.");
            }

            line_view.remove_prefix(content_length_string.size());

            auto [end_ptr, ec] = std::from_chars(line_view.data(), line_view.data() + line_view.size(), content_length);

            line_view.remove_prefix(end_ptr - line_view.data());

            if (ec != std::errc {} || line_view != "\r")
            {
                LOG_WARNING("Invalid Content-Length header received.");
            }

            continue;
        }
        else if (line == "\r")
        {
            // An empty line indicates the end of headers.
            // Go ahead and read the JSON.

            // The >> function left \n as the next character, we need to remove it
            // before we use input.read(). When we use input >> line, all whitespace is
            // ignored, so it is only needed in this case
            input.ignore();

            break;
        }
        else
        {
            // It's another header, ignore it.
        }
    }

    if (content_length > message_size_limit)
    { // 1024M
        LOG_WARNING("Refusing to read message with long Content-Length: ", std::to_string(content_length), ".");
        return false;
    }
    if (content_length == 0)
    {
        LOG_WARNING("Missing Content-Length header, or zero-length message.");
        return false;
    }

    // LSP continues with message of length specified by Content-Length header.
    out.resize(content_length);
    for (std::size_t pos = 0; pos < content_length;)
    {
        input.read(&out[pos], content_length - pos);
        std::streamsize read = input.gcount();
        if (read <= 0)
        {
            LOG_WARNING("Input was aborted. Read only ",
                std::to_string(pos),
                " bytes of expected ",
                std::to_string(content_length));
            return false;
        }
        pos += read;
    }

    return true;
}

std::optional<nlohmann::json> base_protocol_channel::read()
{
    for (;;)
    {
        if (input.fail())
        {
            LOG_ERROR("IO error");
            return std::nullopt;
        }
        if (input.eof())
        {
            LOG_ERROR("IO: unexpected end of file");
            return std::nullopt;
        }

        message_buffer.clear();

        if (read_message(message_buffer))
        {
            LOG_INFO(message_buffer);

            try
            {
                return nlohmann::json::parse(message_buffer);
            }
            catch (const nlohmann::json::exception&)
            {
                LOG_WARNING("Could not parse received JSON: ", message_buffer);
            }
        }
    }
}

} // namespace hlasm_plugin::language_server
