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

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config/proc_grps.h"
#include "library.h"
#include "preprocessor_options.h"
#include "utils/bk_tree.h"
#include "utils/levenshtein_distance.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::config {
struct assembler_options;
struct preprocessor_options;
} // namespace hlasm_plugin::parser_library::config

namespace hlasm_plugin::parser_library::workspaces {

// Represents a named set of libraries (processor_group)
class processor_group
{
public:
    processor_group(const std::string& pg_name,
        const config::assembler_options& asm_options,
        const std::vector<config::preprocessor_options>& pp);

    void copy_diagnostics(std::vector<diagnostic>&) const;

    void add_library(std::shared_ptr<library> library);

    const std::string& name() const { return m_pg_name; }

    std::vector<std::shared_ptr<library>> libraries() const { return m_libs; }

    void apply_options_to(asm_option& opts) const;

    const std::vector<preprocessor_options>& preprocessors() const { return m_prep_opts; }

    void generate_suggestions(bool force = true);
    void invalidate_suggestions();

    std::vector<std::pair<std::string, size_t>> suggest(std::string_view s, bool extended);

    bool refresh_needed(const std::unordered_set<utils::resource::resource_location>& no_filename_rls,
        const std::vector<utils::resource::resource_location>& original_rls) const;

    void add_external_diagnostic(diagnostic d);

private:
    std::vector<std::shared_ptr<library>> m_libs;
    std::map<utils::resource::resource_location, size_t> m_lib_locations;
    std::string m_pg_name;
    config::assembler_options m_asm_opts;
    std::vector<preprocessor_options> m_prep_opts;

    static constexpr size_t suggestion_limit = 32;

    std::optional<utils::bk_tree<std::string, utils::levenshtein_distance_t<suggestion_limit>>> m_suggestions;

    std::vector<diagnostic> m_external_diags;
};
} // namespace hlasm_plugin::parser_library::workspaces
#endif // !HLASMPLUGIN_PARSERLIBRARY_PROCESSOR_GROUP_H
