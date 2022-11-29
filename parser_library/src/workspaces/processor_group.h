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

#include <set>
#include <string>
#include <vector>

#include "config/proc_grps.h"
#include "diagnosable_impl.h"
#include "file_manager.h"
#include "library.h"
#include "preprocessor_options.h"
#include "utils/bk_tree.h"
#include "utils/levenshtein_distance.h"

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
        const std::vector<config::preprocessor_options>& pp);

    void collect_diags() const override;

    void add_library(std::unique_ptr<library> library);

    const std::string& name() const { return m_pg_name; }

    const std::vector<std::unique_ptr<library>>& libraries() const { return m_libs; }

    void apply_options_to(asm_option& opts) const;

    const std::vector<preprocessor_options>& preprocessors() const { return m_prep_opts; }

    void generate_suggestions(bool force = true);

    std::vector<std::pair<std::string, size_t>> suggest(std::string_view s, bool extended);

    bool refresh_needed(const std::vector<utils::resource::resource_location>& urls) const;

private:
    std::vector<std::unique_ptr<library>> m_libs;
    std::string m_pg_name;
    config::assembler_options m_asm_opts;
    std::vector<preprocessor_options> m_prep_opts;

    static constexpr size_t suggestion_limit = 32;

    std::optional<utils::bk_tree<std::string, utils::levenshtein_distance_t<suggestion_limit>>> m_suggestions;

    std::set<std::string, std::less<>> m_refresh_prefix;
};
} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
