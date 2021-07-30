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

#include "analyzer.h"
#include "file_impl.h"
#include "macro_cache.h"
#include "processor.h"

namespace hlasm_plugin::parser_library::workspaces {

class file_manager;

// Implementation of the processor_file interface. Uses analyzer to parse the file
// Then stores it until the next parsing so it is possible to retrieve parsing
// information from it.
class processor_file_impl : public virtual file_impl, public virtual processor_file
{
public:
    processor_file_impl(std::string file_uri, const file_manager& file_mngr, std::atomic<bool>* cancel = nullptr);
    processor_file_impl(file_impl&&, const file_manager& file_mngr, std::atomic<bool>* cancel = nullptr);
    processor_file_impl(const file_impl& file, const file_manager& file_mngr, std::atomic<bool>* cancel = nullptr);
    void collect_diags() const override;
    bool is_once_only() const override;
    // Starts parser with new (empty) context
    parse_result parse(parse_lib_provider&, asm_option, preprocessor_options) override;
    // Starts parser with in the context of parameter
    parse_result parse_macro(parse_lib_provider&, analyzing_context, library_data) override;
    // Starts parser with in the context of parameter, but does not affect LSP, HL info or parse_info_updated.
    // Used by the macro tracer.
    parse_result parse_no_lsp_update(parse_lib_provider&, analyzing_context ctx, library_data) override;

    const std::set<std::string>& dependencies() override;

    const semantics::lines_info& get_hl_info() override;
    const lsp::feature_provider& get_lsp_feature_provider() override;
    const std::set<std::string>& files_to_close() override;
    const performance_metrics& get_metrics() override;

    void erase_cache_of_opencode(const std::string& opencode_file_name) override;

private:
    std::unique_ptr<analyzer> last_analyzer_ = nullptr;
    bool last_analyzer_opencode_ = false;

    bool parse_inner(analyzer&);

    std::atomic<bool>* cancel_;

    std::set<std::string> dependencies_;
    std::set<std::string> files_to_close_;

    macro_cache macro_cache_;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
