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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H

#include <atomic>
#include <memory>
#include <set>

#include "analyzer.h"
#include "fade_messages.h"
#include "file.h"
#include "processing/statement_analyzers/hit_count_analyzer.h"
#include "processor.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

class file_manager;
struct workspace_parse_lib_provider;

// Implementation of the processor_file interface. Uses analyzer to parse the file
// Then stores it until the next parsing so it is possible to retrieve parsing
// information from it.
class processor_file_impl final : public processor_file, public diagnosable_impl
{
    using resource_location = utils::resource::resource_location;
    using resource_location_hasher = utils::resource::resource_location_hasher;
    friend struct workspace_parse_lib_provider; // temporary

public:
    processor_file_impl(std::shared_ptr<file> file, file_manager& file_mngr, std::atomic<bool>* cancel = nullptr);

    void collect_diags() const override;
    bool is_once_only() const override;
    // Starts parser with new (empty) context
    bool parse(parse_lib_provider&, asm_option, std::vector<preprocessor_options>, virtual_file_monitor*) override;

    const std::set<resource_location>& dependencies() override;

    const semantics::lines_info& get_hl_info() override;
    const lsp::lsp_context* get_lsp_context() const override;
    const std::set<resource_location>& files_to_close() override;
    const performance_metrics& get_metrics() override;

    bool has_opencode_lsp_info() const override;
    bool has_macro_lsp_info() const override;

    const std::vector<fade_message_s>& fade_messages() const override;
    const processing::hit_count_map& hit_count_opencode_map() const override;
    const processing::hit_count_map& hit_count_macro_map() const override;

    const file_location& get_location() const override;

    bool current_version() const override;

    void update_source();
    std::shared_ptr<file> current_source() const { return m_file; }

    bool should_collect_hl(context::hlasm_context* ctx = nullptr) const;

private:
    file_manager& m_file_mngr;
    std::shared_ptr<file> m_file;
    std::shared_ptr<context::id_storage> m_last_opencode_id_storage;
    bool m_last_opencode_analyzer_with_lsp = false;
    bool m_last_macro_analyzer_with_lsp = false;

    struct
    {
        semantics::lines_info hl_info;
        std::shared_ptr<lsp::lsp_context> lsp_context;
        std::shared_ptr<const std::vector<fade_message_s>> fade_messages =
            std::make_shared<const std::vector<fade_message_s>>();
        performance_metrics metrics;
        std::vector<virtual_file_handle> vf_handles;
        processing::hit_count_map hc_opencode_map;
        processing::hit_count_map hc_macro_map;
    } m_last_results;

    std::atomic<bool>* m_cancel;

    std::set<resource_location> m_dependencies;
    std::set<resource_location> m_files_to_close;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
