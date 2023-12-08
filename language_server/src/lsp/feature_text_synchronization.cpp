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

#include "feature_text_synchronization.h"

#include <memory>

#include "../logger.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::language_server::lsp {

feature_text_synchronization::feature_text_synchronization(
    parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : feature(response_provider)
    , ws_mngr_(ws_mngr)
{}

void feature_text_synchronization::register_methods(std::map<std::string, method>& methods)
{
    methods.try_emplace("textDocument/didOpen",
        method { [this](const nlohmann::json& args) { on_did_open(args); }, telemetry_log_level::LOG_EVENT });
    methods.try_emplace("textDocument/didChange",
        method { [this](const nlohmann::json& args) { on_did_change(args); }, telemetry_log_level::NO_TELEMETRY });
    methods.try_emplace("textDocument/didClose",
        method { [this](const nlohmann::json& args) { on_did_close(args); }, telemetry_log_level::LOG_EVENT });
}

nlohmann::json feature_text_synchronization::register_capabilities()
{
    return nlohmann::json { {
        "textDocumentSync",
        {
            { "openClose", true },
            { "change", (int)text_document_sync_kind::incremental },
            { "willSave", false },
            { "willSaveWaitUntil", false },
            { "save", false },
        },
    } };
}


void feature_text_synchronization::initialize_feature(const nlohmann::json&)
{
    // No need for initialization in this feature.
}

void feature_text_synchronization::on_did_open(const nlohmann::json& params)
{
    const auto& text_doc = params.at("textDocument");
    const auto& doc_uri = text_doc.at("uri").get_ref<const std::string&>();
    const auto version = text_doc.at("version").get<nlohmann::json::number_unsigned_t>();
    const auto text = text_doc.at("text").get<std::string_view>();

    ws_mngr_.did_open_file(doc_uri.c_str(), version, text.data(), text.size());
}

void feature_text_synchronization::on_did_change(const nlohmann::json& params)
{
    const auto& text_doc = params.at("textDocument");
    const auto& doc_uri = text_doc.at("uri").get_ref<const std::string&>();

    auto version = text_doc.at("version").get<nlohmann::json::number_unsigned_t>();

    const auto& content_changes = params.at("contentChanges");

    std::vector<parser_library::document_change> changes;
    changes.reserve(content_changes.size());
    for (const auto& ch : content_changes)
    {
        const auto text = ch.at("text").get<std::string_view>();

        auto range_it = ch.find("range");
        if (range_it == ch.end())
        {
            changes.emplace_back(text.data(), text.size());
        }
        else
        {
            changes.emplace_back(parse_range(*range_it), text.data(), text.size());
        }
    }
    ws_mngr_.did_change_file(doc_uri.c_str(), version, changes.data(), changes.size());
}

void feature_text_synchronization::on_did_close(const nlohmann::json& params)
{
    const std::string& doc_uri = params.at("textDocument").at("uri").get_ref<const std::string&>();

    ws_mngr_.did_close_file(doc_uri.c_str());
}

} // namespace hlasm_plugin::language_server::lsp
