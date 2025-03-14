/*
 * Copyright (c) 2025 Broadcom.
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

#include "progress_notification.h"

#include <array>
#include <string_view>

#include "nlohmann/json.hpp"
#include "utils/encoding.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::language_server::lsp {

bool progress_notification::client_supports_work_done_progress(const nlohmann::json& params)
{
    static const auto workDoneProgress = "/capabilities/window/workDoneProgress"_json_pointer;
    try
    {
        return params.at(workDoneProgress).get<bool>();
    }
    catch (const nlohmann::json::exception&)
    {
        return false;
    }
}

constexpr auto progress_kind_text = std::to_array<std::string_view>({
    "begin",
    "report",
    "end",
});

nlohmann::json progress_notification::make_progress_notification(long token, progress_kind kind, std::string_view uri)
{
    nlohmann::json result {
        { "token", token },
        {
            "value",
            {
                { "kind", progress_kind_text[static_cast<int>(kind)] },
            },
        },
    };

    auto& value = result["value"];

    if (kind == progress_kind::begin)
        value["title"] = "Parsing";

    if (!uri.empty())
    {
        const utils::resource::resource_location loc(uri);
        value["message"] = utils::encoding::percent_decode(loc.filename());
    }

    return result;
}

void progress_notification::notify(progress_kind kind, std::string_view uri) const
{
    channel->notify("$/progress", make_progress_notification(token, kind, uri));
}

void progress_notification::notify_end()
{
    notify(progress_kind::end);
    ++token;
    token_state = token_state_t::invalid;
}

void progress_notification::parsing_started(std::string_view uri)
{
    pending_uri = uri;

    if (token_state == token_state_t::requested)
        return;

    if (uri.empty())
    {
        if (token_state == token_state_t::valid)
            notify_end();
        return;
    }

    if (token_state == token_state_t::valid)
    {
        notify(progress_kind::report, pending_uri);
        return;
    }

    channel->request(
        "window/workDoneProgress/create",
        nlohmann::json { { "token", token } },
        [this](const nlohmann::json&) {
            token_state = token_state_t::valid;
            if (pending_uri.empty())
                notify_end();
            else
                notify(progress_kind::begin, pending_uri);
        },
        [this](int, const char*) { token_state = token_state_t::invalid; });
    token_state = token_state_t::requested;
}


} // namespace hlasm_plugin::language_server::lsp
