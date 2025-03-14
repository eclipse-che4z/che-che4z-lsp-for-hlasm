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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_PROGRESS_NOTIFICATION_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_PROGRESS_NOTIFICATION_H

#include <string_view>

#include "../feature.h"
#include "nlohmann/json_fwd.hpp"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::lsp {

class progress_notification final : public parser_library::progress_notification_consumer
{
    response_provider* channel;
    std::string pending_uri;
    long token = 0;

    enum class token_state_t
    {
        invalid,
        valid,
        requested,
    };
    token_state_t token_state = token_state_t::invalid;

    enum class progress_kind
    {
        begin,
        report,
        end,
    };

    void notify_end();
    void notify(progress_kind kind, std::string_view uri = {}) const;
    static nlohmann::json make_progress_notification(long token, progress_kind kind, std::string_view uri = {});

public:
    static bool client_supports_work_done_progress(const nlohmann::json& initialization_parameters);
    void parsing_started(std::string_view uri) override;

    explicit constexpr progress_notification(response_provider& channel) noexcept
        : channel(&channel)
    {}
};

} // namespace hlasm_plugin::language_server::lsp

#endif
