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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H

// This file specifies the interface of parser library.
// The pimpl (pointer to implementation) idiom is used to hide its implementation.
// It implements LSP requests and notifications and is used by the language server.

#include <atomic>
#include <memory>
#include <span>
#include <utility>

#include "branch_info.h"
#include "completion_trigger_kind.h"
#include "folding_range.h"
#include "lib_config.h"
#include "location.h"
#include "message_consumer.h"
#include "protocol.h"
#include "workspace_manager_requests.h"

namespace hlasm_plugin::parser_library {
struct completion_item;
struct diagnostic;
struct document_symbol_item;
struct fade_message;
class workspace_manager_external_file_requests;
class external_configuration_requests;
class watcher_registration_provider;
namespace debugging {
struct debugger_configuration;
struct output_line;
} // namespace debugging

// Interface that can be implemented to be able to get list of
// diagnostics from workspace manager whenever a file is parsed
// Passes list of all diagnostics that are in all currently opened files.
class diagnostics_consumer
{
public:
    virtual void consume_diagnostics(
        std::span<const diagnostic> diagnostics, std::span<const fade_message> fade_messages) = 0;

protected:
    ~diagnostics_consumer() = default;
};

// Interface that can be implemented to be able to get performance metrics
//(time that parsing took, number of parsed lines, etc)
class parsing_metadata_consumer
{
public:
    virtual void consume_parsing_metadata(std::string_view uri, double duration, const parsing_metadata& metrics) = 0;
    virtual void outputs_changed(std::string_view uri) = 0;

protected:
    ~parsing_metadata_consumer() = default;
};

class progress_notification_consumer
{
public:
    virtual void parsing_started(std::string_view uri) = 0;

protected:
    ~progress_notification_consumer() = default;
};

enum class fs_change_type
{
    invalid = 0,
    created = 1,
    changed = 2,
    deleted = 3,
};

struct fs_change
{
    std::string_view uri;
    fs_change_type change_type;
};

struct opcode_suggestion
{
    std::string opcode;
    size_t distance;
};

template<typename T>
class workspace_manager_response;

class debugger_configuration_provider
{
protected:
    ~debugger_configuration_provider() = default;

public:
    virtual void provide_debugger_configuration(
        std::string_view document_uri, workspace_manager_response<debugging::debugger_configuration> conf) = 0;
};

class workspace_manager
{
public:
    virtual ~workspace_manager() = default;
    virtual void add_workspace(std::string_view name, std::string_view uri) = 0;
    virtual void remove_workspace(std::string_view uri) = 0;

    virtual void did_open_file(std::string_view document_uri, version_t version, std::string_view text) = 0;
    virtual void did_change_file(
        std::string_view document_uri, version_t version, std::span<const document_change> changes) = 0;
    virtual void did_close_file(std::string_view document_uri) = 0;
    virtual void did_change_watched_files(std::span<const fs_change> changes) = 0;

    virtual void definition(
        std::string_view document_uri, position pos, workspace_manager_response<const location&> resp) = 0;
    virtual void references(
        std::string_view document_uri, position pos, workspace_manager_response<std::span<const location>> resp) = 0;
    virtual void hover(
        std::string_view document_uri, position pos, workspace_manager_response<std::string_view> resp) = 0;
    virtual void completion(std::string_view document_uri,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind,
        workspace_manager_response<std::span<const completion_item>> resp) = 0;

    virtual void semantic_tokens(
        std::string_view document_uri, workspace_manager_response<std::span<const token_info>> resp) = 0;
    virtual void document_symbol(
        std::string_view document_uri, workspace_manager_response<std::span<const document_symbol_item>> resp) = 0;

    virtual void configuration_changed(const lib_config& new_config, std::string_view full_cfg) = 0;

    // implementation of observer pattern - register consumer.
    virtual void register_diagnostics_consumer(diagnostics_consumer* consumer) = 0;
    virtual void unregister_diagnostics_consumer(diagnostics_consumer* consumer) = 0;
    virtual void register_parsing_metadata_consumer(parsing_metadata_consumer* consumer) = 0;
    virtual void unregister_parsing_metadata_consumer(parsing_metadata_consumer* consumer) = 0;
    virtual void set_message_consumer(message_consumer* consumer) = 0;
    virtual void set_request_interface(workspace_manager_requests* requests) = 0;
    virtual void set_progress_notification_consumer(progress_notification_consumer* progress) = 0;
    virtual void set_watcher_registration_provider(watcher_registration_provider* watchers) = 0;

    virtual std::string get_virtual_file_content(unsigned long long id) const = 0;

    virtual void toggle_advisory_configuration_diagnostics() = 0;

    virtual void make_opcode_suggestion(std::string_view document_uri,
        std::string_view opcode,
        bool extended,
        workspace_manager_response<std::span<const opcode_suggestion>>) = 0;

    virtual void idle_handler(const std::atomic<unsigned char>* yield_indicator = nullptr) = 0;

    virtual debugger_configuration_provider& get_debugger_configuration_provider() = 0;

    virtual void invalidate_external_configuration(std::string_view uri) = 0;

    virtual void branch_information(
        std::string_view document_uri, workspace_manager_response<std::span<const branch_info>> resp) = 0;

    virtual void folding(
        std::string_view document_uri, workspace_manager_response<std::span<const folding_range>> resp) = 0;

    virtual void retrieve_output(
        std::string_view document_uri, workspace_manager_response<std::span<const output_line>> resp) = 0;

    virtual void change_implicit_group_base(std::string_view uri) = 0;
};

workspace_manager* create_workspace_manager_impl(
    workspace_manager_external_file_requests* external_requests, bool vscode_extensions);
inline std::unique_ptr<workspace_manager> create_workspace_manager(
    workspace_manager_external_file_requests* external_requests = nullptr, bool vscode_extensions = false)
{
    return std::unique_ptr<workspace_manager>(create_workspace_manager_impl(external_requests, vscode_extensions));
}

} // namespace hlasm_plugin::parser_library
#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
