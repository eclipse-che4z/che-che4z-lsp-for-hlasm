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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H

#include <atomic>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "diagnosable_impl.h"
#include "file_manager_vfm.h"
#include "lib_config.h"
#include "message_consumer.h"
#include "processor.h"
#include "processor_group.h"
#include "utils/resource_location.h"
#include "workspace_configuration.h"

namespace hlasm_plugin::parser_library::workspaces {
class file_manager;
using ws_uri = std::string;
using ws_highlight_info = std::unordered_map<std::string, semantics::highlighting_info>;

// Represents a LSP workspace. It solves all dependencies between files -
// implements parse lib provider and decides which files are to be parsed
// when a particular file has been changed in the editor.
class workspace : public diagnosable_impl, public parse_lib_provider
{
public:
    // Creates just a dummy workspace with no libraries - no dependencies
    // between files.
    workspace(file_manager& file_manager,
        const lib_config& global_config,
        const shared_json& global_settings,
        std::atomic<bool>* cancel = nullptr);
    workspace(const utils::resource::resource_location& location,
        file_manager& file_manager,
        const lib_config& global_config,
        const shared_json& global_settings,
        std::atomic<bool>* cancel = nullptr);
    workspace(const utils::resource::resource_location& location,
        const std::string& name,
        file_manager& file_manager,
        const lib_config& global_config,
        const shared_json& global_settings,
        std::atomic<bool>* cancel = nullptr);

    workspace(const workspace& ws) = delete;
    workspace& operator=(const workspace&) = delete;

    workspace(workspace&& ws) = default;
    workspace& operator=(workspace&&) = delete;

    void collect_diags() const override;

    workspace_file_info parse_file(
        const utils::resource::resource_location& file_location, open_file_result file_content_status);
    workspace_file_info parse_successful(const processor_file_ptr& f);
    bool refresh_libraries(const std::vector<utils::resource::resource_location>& file_locations);
    workspace_file_info did_open_file(const utils::resource::resource_location& file_location,
        open_file_result file_content_status = open_file_result::changed_content);
    void did_close_file(const utils::resource::resource_location& file_location);
    void did_change_file(
        const utils::resource::resource_location& file_location, const document_change* changes, size_t ch_size);
    void did_change_watched_files(const std::vector<utils::resource::resource_location>& file_locations);

    location definition(const utils::resource::resource_location& document_loc, position pos) const;
    location_list references(const utils::resource::resource_location& document_loc, position pos) const;
    std::string hover(const utils::resource::resource_location& document_loc, position pos) const;
    lsp::completion_list_s completion(const utils::resource::resource_location& document_loc,
        position pos,
        char trigger_char,
        completion_trigger_kind trigger_kind);
    lsp::document_symbol_list_s document_symbol(
        const utils::resource::resource_location& document_loc, long long limit) const;

    parse_result parse_library(const std::string& library, analyzing_context ctx, library_data data) override;
    bool has_library(const std::string& library, const utils::resource::resource_location& program) const override;
    std::optional<std::string> get_library(const std::string& library,
        const utils::resource::resource_location& program,
        std::optional<utils::resource::resource_location>& location) const override;
    virtual asm_option get_asm_options(const utils::resource::resource_location& file_location) const;
    virtual std::vector<preprocessor_options> get_preprocessor_options(
        const utils::resource::resource_location& file_location) const;
    const ws_uri& uri() const;

    void open();
    void close();

    void set_message_consumer(message_consumer* consumer);

    processor_file_ptr get_processor_file(const utils::resource::resource_location& file_location);

    file_manager& get_file_manager();

    bool settings_updated();

    const processor_group& get_proc_grp_by_program(const utils::resource::resource_location& file) const;
    const processor_group& get_proc_grp(const proc_grp_id& id) const; // test only

    std::vector<std::pair<std::string, size_t>> make_opcode_suggestion(
        const utils::resource::resource_location& file, std::string_view opcode, bool extended);

    static lsp::completion_list_s generate_completion(const lsp::completion_list_source& cls,
        std::function<std::vector<std::string>(std::string_view)> instruction_suggestions = {});

private:
    std::atomic<bool>* cancel_;

    std::string name_;
    utils::resource::resource_location location_;
    file_manager& file_manager_;
    file_manager_vfm fm_vfm_;

    processor_group implicit_proc_grp;

    bool opened_ = false;

    void reparse_after_config_refresh();

    // files, that depend on others (e.g. open code files that use macros)
    std::set<utils::resource::resource_location> dependants_;

    struct opened_file_details
    {
        opened_file_details() = default;
        explicit opened_file_details(utils::resource::resource_location alternative_config)
            : alternative_config(std::move(alternative_config))
        {}
        utils::resource::resource_location alternative_config;
    };

    std::unordered_map<utils::resource::resource_location,
        opened_file_details,
        utils::resource::resource_location_hasher>
        opened_files_;

    void filter_and_close_dependencies_(
        const std::set<utils::resource::resource_location>& dependencies, processor_file_ptr file);
    bool is_dependency_(const utils::resource::resource_location& file_location);

    std::vector<processor_file_ptr> find_related_opencodes(
        const utils::resource::resource_location& document_loc) const;
    void delete_diags(processor_file_ptr file);

    void show_message(const std::string& message);

    message_consumer* message_consumer_ = nullptr;

    const lib_config& global_config_;

    lib_config get_config() const;

    workspace_configuration m_configuration;

    static lsp::completion_list_s generate_completion(
        std::monostate, const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions);
    static lsp::completion_list_s generate_completion(const lsp::vardef_storage*,
        const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions);
    static lsp::completion_list_s generate_completion(const context::label_storage*,
        const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions);
    static lsp::completion_list_s generate_completion(const lsp::completion_list_instructions&,
        const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions);

    std::vector<processor_file_ptr> collect_dependants(const utils::resource::resource_location& file_location) const;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
