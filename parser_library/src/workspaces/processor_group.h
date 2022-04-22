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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H

#include "compiler_options.h"
#include "diagnosable_impl.h"
#include "file_manager.h"
#include "library.h"
#include "preprocessor_options.h"

namespace hlasm_plugin::parser_library::config {
struct assembler_options;
struct preprocessor_options;
} // namespace hlasm_plugin::parser_library::config

namespace hlasm_plugin::parser_library::workspaces {

// Represents a named set of libraries (processor_group)
class processor_group : public diagnosable_impl
{
public:
    processor_group(const std::string& pg_name,
        std::string_view pg_file_name,
        const config::assembler_options& asm_options,
        const config::preprocessor_options& pp);

    void collect_diags() const override;

    void add_library(std::unique_ptr<library> library);

    const std::string& name() const { return m_pg_name; }

    const std::vector<std::unique_ptr<library>>& libraries() const { return m_libs; }

    const asm_option& asm_options() const { return m_asm_opts; }

    const preprocessor_options& preprocessor() const { return m_prep_opts; }

private:
    std::vector<std::unique_ptr<library>> m_libs;
    std::string m_pg_name;
    asm_option m_asm_opts;
    preprocessor_options m_prep_opts;

    instruction_set_version find_instruction_set(std::string_view optable, std::string_view pg_file_name) const;
    asm_option translate_assembler_options(
        const config::assembler_options& asm_options, std::string_view pg_file_name) const;
};
} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
