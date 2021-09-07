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

#include "../logger.h"
#include "protocol.h"
namespace hlasm_plugin::language_server::lsp {

feature_text_synchronization::feature_text_synchronization(
    parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : feature(ws_mngr, response_provider)
{}

void feature_text_synchronization::register_methods(std::map<std::string, method>& methods)
{
    methods.try_emplace("textDocument/didOpen",
        method { [this](const json& id, const json& args) { on_did_open(id, args); },
            telemetry_log_level::LOG_WITH_METRICS });
    methods.try_emplace("textDocument/didChange",
        method {
            [this](const json& id, const json& args) { on_did_change(id, args); }, telemetry_log_level::NO_TELEMETRY });
    methods.try_emplace("textDocument/didClose",
        method {
            [this](const json& id, const json& args) { on_did_close(id, args); }, telemetry_log_level::LOG_EVENT });
}

json feature_text_synchronization::register_capabilities()
{
    // there is no reason why not ask for notifications (most of them is
    // ignored anyway).
    // we cant process willSaveWaitUntil because it is a request and we dont
    // want many hanging requests
    return json { { "textDocumentSync",
        json { { "openClose", true },
            { "change", (int)text_document_sync_kind::incremental },
            { "willSave", true },
            { "willSaveWaitUntil", false },
            { "save", json { { "includeText", true } } } } } };
}


void feature_text_synchronization::initialize_feature(const json&)
{
    // No need for initialization in this feature.
}

void feature_text_synchronization::on_did_open(const json&, const json& params)
{
    json text_doc = params["textDocument"];
    std::string doc_uri = text_doc["uri"].get<std::string>();
    auto version = text_doc["version"].get<nlohmann::json::number_unsigned_t>();
    std::string text = text_doc["text"].get<std::string>();

    auto path = uri_to_path(doc_uri);

    if (path.empty())
    {
        LOG_WARNING("Ignoring on_did_open with unsupported scheme for document URI. URI was " + doc_uri);
        return;
    }

    ws_mngr_.did_open_file(path.c_str(), version, text.c_str(), text.size());
}

void feature_text_synchronization::on_did_change(const json&, const json& params)
{
    json text_doc = params["textDocument"];
    std::string doc_uri = text_doc["uri"].get<std::string>();

    auto version = text_doc["version"].get<nlohmann::json::number_unsigned_t>();

    json content_changes = params["contentChanges"];

    std::vector<parser_library::document_change> changes;
    std::vector<std::string> texts(content_changes.size());
    size_t i = 0;
    for (auto& ch : content_changes)
    {
        texts[i] = ch["text"].get<std::string>();

        auto range_it = ch.find("range");
        if (range_it == ch.end())
        {
            changes.emplace_back(texts[i].c_str(), texts[i].size());
        }
        else
        {
            changes.emplace_back(parse_range(ch["range"]), texts[i].c_str(), texts[i].size());
        }

        ++i;
    }
    ws_mngr_.did_change_file(uri_to_path(doc_uri).c_str(), version, &*changes.begin(), changes.size());
}

void feature_text_synchronization::on_did_close(const json&, const json& params)
{
    std::string uri = params["textDocument"]["uri"].get<std::string>();

    ws_mngr_.did_close_file(uri_to_path(uri).c_str());
}

} // namespace hlasm_plugin::language_server::lsp
