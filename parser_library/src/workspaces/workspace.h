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
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "diagnosable_impl.h"
#include "file_manager_vfm.h"
#include "lib_config.h"
#include "macro_cache.h"
#include "message_consumer.h"
#include "processor.h"
#include "processor_group.h"
#include "utils/resource_location.h"
#include "workspace_configuration.h"

namespace hlasm_plugin::parser_library::workspaces {
class file_manager;
class library;
class processor_file_impl;
using ws_uri = std::string;
using ws_highlight_info = std::unordered_map<std::string, semantics::highlighting_info>;
struct workspace_parse_lib_provider;
// Represents a LSP workspace. It solves all dependencies between files -
// implements parse lib provider and decides which files are to be parsed
// when a particular file has been changed in the editor.
class workspace : public diagnosable_impl
{
public:
    using resource_location = utils::resource::resource_location;
    using resource_location_hasher = utils::resource::resource_location_hasher;
    // Creates just a dummy workspace with no libraries - no dependencies
    // between files.
    workspace(file_manager& file_manager,
        const lib_config& global_config,
        const shared_json& global_settings,
        std::atomic<bool>* cancel = nullptr,
        std::shared_ptr<library> implicit_library = nullptr);
    workspace(const resource_location& location,
        file_manager& file_manager,
        const lib_config& global_config,
        const shared_json& global_settings,
        std::atomic<bool>* cancel = nullptr);
    workspace(const resource_location& location,
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

    workspace_file_info parse_file(const resource_location& file_location, open_file_result file_content_status);
    bool refresh_libraries(const std::vector<resource_location>& file_locations);
    workspace_file_info did_open_file(const resource_location& file_location,
        open_file_result file_content_status = open_file_result::changed_content);
    void did_close_file(const resource_location& file_location);
    void did_change_file(const resource_location& file_location, const document_change* changes, size_t ch_size);
    void did_change_watched_files(const std::vector<resource_location>& file_locations);

    location definition(const resource_location& document_loc, position pos) const;
    std::vector<location> references(const resource_location& document_loc, position pos) const;
    std::string hover(const resource_location& document_loc, position pos) const;
    std::vector<lsp::completion_item_s> completion(
        const resource_location& document_loc, position pos, char trigger_char, completion_trigger_kind trigger_kind);
    std::vector<lsp::document_symbol_item_s> document_symbol(
        const resource_location& document_loc, long long limit) const;

    std::vector<token_info> semantic_tokens(const resource_location& document_loc) const;

    virtual std::vector<std::shared_ptr<library>> get_libraries(const resource_location& file_location) const;
    virtual asm_option get_asm_options(const resource_location& file_location) const;
    virtual std::vector<preprocessor_options> get_preprocessor_options(const resource_location& file_location) const;
    const ws_uri& uri() const;

    void open();
    void close();

    void set_message_consumer(message_consumer* consumer);

    std::shared_ptr<processor_file> find_processor_file(const resource_location& file) const;

    file_manager& get_file_manager() const;

    bool settings_updated();

    const processor_group& get_proc_grp_by_program(const resource_location& file) const;
    const processor_group& get_proc_grp(const proc_grp_id& id) const; // test only

    std::vector<std::pair<std::string, size_t>> make_opcode_suggestion(
        const resource_location& file, std::string_view opcode, bool extended);

    void retrieve_fade_messages(std::vector<fade_message_s>& fms) const;

private:
    std::atomic<bool>* cancel_;

    std::string name_;
    resource_location location_;
    file_manager& file_manager_;
    file_manager_vfm fm_vfm_;

    processor_group implicit_proc_grp;

    bool opened_ = false;

    void reparse_after_config_refresh();

    void filter_and_close_dependencies_(std::set<resource_location> dependencies, std::shared_ptr<processor_file> file);
    bool is_dependency_(const resource_location& file_location) const;

    std::vector<std::shared_ptr<processor_file>> find_related_opencodes(const resource_location& document_loc) const;
    void delete_diags(std::shared_ptr<processor_file> file);

    void show_message(const std::string& message);

    message_consumer* message_consumer_ = nullptr;

    const lib_config& global_config_;

    lib_config get_config() const;

    workspace_configuration m_configuration;

    struct processor_file_compoments
    {
        std::shared_ptr<processor_file_impl> m_processor_file;
        std::unordered_map<resource_location,
            std::shared_ptr<std::pair<version_t, macro_cache>>,
            resource_location_hasher>
            m_macro_cache;

        resource_location m_alternative_config = resource_location();

        bool m_opened = false;

        void update_source_if_needed() const;
    };

    std::unordered_map<resource_location, processor_file_compoments, resource_location_hasher> m_processor_files;

    std::vector<processor_file_compoments*> populate_files_to_parse(
        const utils::resource::resource_location& file_location, open_file_result file_content_status);

    processor_file_compoments& add_processor_file_impl(const resource_location& file);
    processor_file_compoments* find_processor_file_impl(const resource_location& file);
    const processor_file_compoments* find_processor_file_impl(const resource_location& file) const;
    friend struct workspace_parse_lib_provider;
    workspace_file_info parse_successful(processor_file_compoments& comp, workspace_parse_lib_provider libs);
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_H
