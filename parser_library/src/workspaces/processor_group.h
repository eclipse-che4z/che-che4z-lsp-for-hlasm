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

#include "config/proc_grps.h"
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
        const config::assembler_options& asm_options,
        const config::preprocessor_options& pp);

    void collect_diags() const override;

    void add_library(std::unique_ptr<library> library);

    const std::string& name() const { return m_pg_name; }

    const std::vector<std::unique_ptr<library>>& libraries() const { return m_libs; }

    void update_asm_options(asm_option& opts) const;

    const preprocessor_options& preprocessor() const { return m_prep_opts; }

private:
    std::vector<std::unique_ptr<library>> m_libs;
    std::string m_pg_name;
    config::assembler_options m_asm_opts;
    preprocessor_options m_prep_opts;
};
} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
