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

// Pimpl implementations of workspace manager. Also converts C-like parameters
// into C++ ones.

#include "workspace_manager.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "utils/resource_location.h"
#include "workspace_manager_impl.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library {

workspace_manager::workspace_manager(workspace_manager_external_file_requests* external_file_requests)
    : impl_(new impl(external_file_requests))
{}

workspace_manager::workspace_manager(workspace_manager&& ws_mngr) noexcept
    : impl_(ws_mngr.impl_)
{
    ws_mngr.impl_ = nullptr;
}
workspace_manager& workspace_manager::operator=(workspace_manager&& ws_mngr) noexcept
{
    std::swap(impl_, ws_mngr.impl_);
    return *this;
}

workspace_manager::~workspace_manager() { delete impl_; }

size_t workspace_manager::get_workspaces(ws_id* workspaces, size_t max_size)
{
    return impl_->get_workspaces(workspaces, max_size);
}

size_t workspace_manager::get_workspaces_count() { return impl_->get_workspaces_count(); }

void workspace_manager::add_workspace(const char* name, const char* uri) { impl_->add_workspace(name, uri); }

ws_id workspace_manager::find_workspace(const char* document_uri) { return impl_->find_workspace(document_uri); }

void workspace_manager::remove_workspace(const char* uri) { impl_->remove_workspace(uri); }

void workspace_manager::did_change_watched_files(sequence<fs_change> fs_changes)
{
    std::vector<utils::resource::resource_location> paths_s;
    paths_s.reserve(fs_changes.size());
    std::transform(fs_changes.begin(), fs_changes.end(), std::back_inserter(paths_s), [](const auto& change) {
        return utils::resource::resource_location(static_cast<std::string_view>(change.uri));
    });
    impl_->did_change_watched_files(std::move(paths_s));
}

void workspace_manager::did_open_file(const char* document_uri, version_t version, const char* text, size_t text_size)
{
    impl_->did_open_file(utils::resource::resource_location(document_uri), version, std::string(text, text_size));
}
void workspace_manager::did_change_file(
    const char* document_uri, version_t version, const document_change* changes, size_t ch_size)
{
    impl_->did_change_file(utils::resource::resource_location(document_uri), version, changes, ch_size);
}

void workspace_manager::did_close_file(const char* document_uri)
{
    impl_->did_close_file(utils::resource::resource_location(document_uri));
}

void workspace_manager::configuration_changed(const lib_config& new_config)
{
    impl_->configuration_changed(new_config);
}

void workspace_manager::register_diagnostics_consumer(diagnostics_consumer* consumer)
{
    impl_->register_diagnostics_consumer(consumer);
}

void workspace_manager::unregister_diagnostics_consumer(diagnostics_consumer* consumer)
{
    impl_->unregister_diagnostics_consumer(consumer);
}

void workspace_manager::register_parsing_metadata_consumer(parsing_metadata_consumer* consumer)
{
    impl_->register_parsing_metadata_consumer(consumer);
}

void workspace_manager::unregister_parsing_metadata_consumer(parsing_metadata_consumer* consumer)
{
    impl_->unregister_parsing_metadata_consumer(consumer);
}

void workspace_manager::set_message_consumer(message_consumer* consumer) { impl_->set_message_consumer(consumer); }

void workspace_manager::set_request_interface(workspace_manager_requests* requests)
{
    impl_->set_request_interface(requests);
}

void workspace_manager::definition(const char* document_uri, position pos, workspace_manager_response<position_uri> r)
{
    impl_->definition(document_uri, pos, std::move(r));
}

void workspace_manager::references(
    const char* document_uri, position pos, workspace_manager_response<position_uri_list> r)
{
    impl_->references(document_uri, pos, std::move(r));
}

void workspace_manager::hover(const char* document_uri, position pos, workspace_manager_response<sequence<char>> r)
{
    impl_->hover(document_uri, pos, std::move(r));
}

void workspace_manager::completion(const char* document_uri,
    position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind,
    workspace_manager_response<completion_list> r)
{
    impl_->completion(document_uri, pos, trigger_char, trigger_kind, std::move(r));
}

void workspace_manager::semantic_tokens(
    const char* document_uri, workspace_manager_response<continuous_sequence<token_info>> r)
{
    impl_->semantic_tokens(document_uri, std::move(r));
}

void workspace_manager::document_symbol(
    const char* document_uri, long long limit, workspace_manager_response<document_symbol_list> r)
{
    impl_->document_symbol(document_uri, limit, std::move(r));
}

continuous_sequence<char> workspace_manager::get_virtual_file_content(unsigned long long id) const
{
    return impl_->get_virtual_file_content(id);
}

void workspace_manager::make_opcode_suggestion(const char* document_uri,
    const char* opcode,
    bool extended,
    workspace_manager_response<continuous_sequence<opcode_suggestion>> r) const
{
    impl_->make_opcode_suggestion(document_uri, opcode, extended, std::move(r));
}


bool workspace_manager::idle_handler(const std::atomic<unsigned char>* yield_indicator)
{
    return impl_->idle_handler(yield_indicator);
}

} // namespace hlasm_plugin::parser_library
