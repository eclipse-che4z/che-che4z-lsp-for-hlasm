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
#include "processor.h"

namespace hlasm_plugin::parser_library::workspaces {

// Implementation of the processor_file interface. Uses analyzer to parse the file
// Then stores it until the next parsing so it is possible to retrieve parsing
// information from it.
class processor_file_impl : public virtual file_impl, public virtual processor_file
{
public:
    processor_file_impl(std::string file_uri, std::atomic<bool>* cancel = nullptr);
    processor_file_impl(file_impl&&, std::atomic<bool>* cancel = nullptr);
    processor_file_impl(const file_impl& file, std::atomic<bool>* cancel = nullptr);
    void collect_diags() const override;
    bool is_once_only() const override;
    // Starts parser with new (empty) context
    virtual parse_result parse(parse_lib_provider&) override;
    // Starts parser with in the context of parameter
    virtual parse_result parse_macro(parse_lib_provider&, context::hlasm_context&, const library_data) override;
    // Starts parser with in the context of parameter, but does not affect LSP, HL info or parse_info_updated.
    // Used by the macro tracer.
    virtual parse_result parse_no_lsp_update(parse_lib_provider&, context::hlasm_context&, const library_data) override;

    // Returns true if parsing occured since this method was called last.
    bool parse_info_updated() override;

    const std::set<std::string>& dependencies() override;

    virtual ~processor_file_impl() = default;
    virtual const file_highlighting_info get_hl_info() override;
    virtual const semantics::lsp_info_processor get_lsp_info() override;
    virtual const std::set<std::string>& files_to_close() override;
    virtual const performance_metrics& get_metrics() override;

private:
    std::unique_ptr<analyzer> analyzer_;
    // This is here only because CA expressions need parser to be alive to evaluate
    std::unique_ptr<analyzer> no_update_analyzer_;

    bool parse_inner(analyzer&);

    bool parse_info_updated_ = false;
    std::atomic<bool>* cancel_;

    std::set<std::string> dependencies_;
    std::set<std::string> files_to_close_;
};

} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_FILE_H
