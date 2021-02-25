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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_CHANNEL_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_CHANNEL_H

#include <optional>

#include "json.hpp"

namespace hlasm_plugin::language_server {

class json_source
{
public:
    virtual std::optional<nlohmann::json> read() = 0;

protected:
    ~json_source() = default;
};

class json_sink
{
public:
    virtual void write(const nlohmann::json&) = 0;
    virtual void write(nlohmann::json&&) = 0;

protected:
    ~json_sink() = default;
};

class json_channel : public json_source, public json_sink
{
protected:
    ~json_channel() = default;
};

class json_channel_adapter final : public json_channel
{
    json_source& source;
    json_sink& sink;

public:
    json_channel_adapter(json_channel& ch)
        : source(ch)
        , sink(ch)
    {}
    json_channel_adapter(json_source& in, json_sink& out)
        : source(in)
        , sink(out)
    {}
    std::optional<nlohmann::json> read() override { return source.read(); }
    void write(const nlohmann::json& j) override { sink.write(j); }
    void write(nlohmann::json&& j) override { sink.write(std::move(j)); }
};

} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_JSON_CHANNEL_H