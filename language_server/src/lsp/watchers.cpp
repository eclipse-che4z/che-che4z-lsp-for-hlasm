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

#include "watchers.h"

#include "nlohmann/json.hpp"
#include "utils/resource_location.h"

namespace hlasm_plugin::language_server::lsp {

enum class watch_kind
{
    _default = 0,
    _create = 1,
    _change = 2,
    _delete = 4,
};

[[nodiscard]] constexpr watch_kind operator|(watch_kind l, watch_kind r) noexcept
{
    return static_cast<watch_kind>(static_cast<int>(l) | static_cast<int>(r));
}

struct watcher
{
    std::string_view pattern;
    std::string_view base = {};
    watch_kind kind = watch_kind::_default;
};

void to_json(nlohmann::json& j, const watcher& w)
{
    if (w.base.empty())
        j["globPattern"] = w.pattern;
    else
    {
        j["globPattern"] = {
            { "baseUri", w.base },
            { "pattern", w.pattern },
        };
    }
    if (w.kind != watch_kind {})
        j["kind"] = w.kind;
}

nlohmann::json watcher_registration_base(parser_library::watcher_registration_id id)
{
    return {
        { "id", "watcher_" + std::to_string(static_cast<std::underlying_type_t<decltype(id)>>(id)) },
        { "method", "workspace/didChangeWatchedFiles" },
    };
}

nlohmann::json make_watcher_list(std::string_view uri, bool r)
{
    nlohmann::json::array_t watchers;
    utils::resource::resource_location loc(uri);
    loc.join("");
    watchers.emplace_back(watcher { .pattern = r ? "**/*" : "*", .base = loc.get_uri() });
    if (auto parent = utils::resource::resource_location::join(loc, "..").lexically_normal(); parent != loc)
    {
        const auto rel = loc.lexically_relative(parent);
        const auto rel_uri = rel.get_uri();

        watchers.emplace_back(watcher {
            .pattern = rel_uri.substr(0, rel_uri.size() - 1),
            .base = parent.get_uri(),
            .kind = watch_kind::_create | watch_kind::_delete,
        });
    }

    return watchers;
}

nlohmann::json watcher_registeration(parser_library::watcher_registration_id id, std::string_view uri, bool r)
{
    auto result = watcher_registration_base(id);

    result["registerOptions"] = { { "watchers", make_watcher_list(uri, r) } };

    return result;
}

nlohmann::json watcher_unregisteration(parser_library::watcher_registration_id id)
{
    return watcher_registration_base(id);
}

nlohmann::json default_watcher_registration()
{
    return nlohmann::json {
        { "id", "global_watchers" },
        { "method", "workspace/didChangeWatchedFiles" },
        {
            "registerOptions",
            {
                {
                    "watchers",
                    std::to_array({
                        watcher { .pattern = "**/*" },
                        watcher { .pattern = ".hlasmplugin/*.json" },
                    }),
                },
            },
        },
    };
}
} // namespace hlasm_plugin::language_server::lsp
