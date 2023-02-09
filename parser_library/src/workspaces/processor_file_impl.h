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
#include "macro_cache.h"
#include "processor.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

class file_manager;

// Implementation of the processor_file interface. Uses analyzer to parse the file
// Then stores it until the next parsing so it is possible to retrieve parsing
// information from it.
class processor_file_impl final : public processor_file, public diagnosable_impl
{
public:
    processor_file_impl(std::shared_ptr<file> file, file_manager& file_mngr, std::atomic<bool>* cancel = nullptr);

    void collect_diags() const override;
    bool is_once_only() const override;
    // Starts parser with new (empty) context
    parse_result parse(
        parse_lib_provider&, asm_option, std::vector<preprocessor_options>, virtual_file_monitor*) override;
    // Starts parser with in the context of parameter
    parse_result parse_macro(parse_lib_provider&, analyzing_context, library_data) override;

    const std::set<utils::resource::resource_location>& dependencies() override;

    const semantics::lines_info& get_hl_info() override;
    const lsp::lsp_context* get_lsp_context() override;
    const std::set<utils::resource::resource_location>& files_to_close() override;
    const performance_metrics& get_metrics() override;

    void erase_unused_cache_entries() override;

    bool has_lsp_info() const override;

    void retrieve_fade_messages(std::vector<fade_message_s>& fms) const override;
    const file_location& get_location() const override;

    bool current_version() const override;

    void update_source();
    std::shared_ptr<file> current_source() const { return file_; }
    void store_used_files(std::unordered_map<utils::resource::resource_location,
        std::shared_ptr<file>,
        utils::resource::resource_location_hasher> used_files);

private:
    file_manager& file_mngr_;
    std::shared_ptr<file> file_;
    std::unique_ptr<analyzer> last_analyzer_;
    std::shared_ptr<context::id_storage> last_opencode_id_storage_;
    bool last_analyzer_with_lsp = false;

    semantics::lines_info last_hl_info_;

    std::atomic<bool>* cancel_;

    std::set<utils::resource::resource_location> dependencies_;
    std::set<utils::resource::resource_location> files_to_close_;

    std::unordered_map<utils::resource::resource_location,
        std::shared_ptr<file>,
        utils::resource::resource_location_hasher>
        used_files;

    macro_cache macro_cache_;

    std::shared_ptr<const std::vector<fade_message_s>> fade_messages_ =
        std::make_shared<const std::vector<fade_message_s>>();

    bool should_collect_hl(context::hlasm_context* ctx = nullptr) const;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
