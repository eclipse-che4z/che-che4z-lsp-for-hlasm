/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_MACRO_CACHE_H
#define HLASMPLUGIN_PARSERLIBRARY_MACRO_CACHE_H

#include "analyzer.h"

namespace hlasm_plugin::parser_library::workspaces {

class file_manager;
class file;

struct cached_opsyn_mnemo
{
    context::id_index from_instr;
    context::id_index to_instr;
    bool is_macro;
};

// Contains all the context that affects parsing an external file (macro or copy member)
struct macro_cache_key
{
    [[nodiscard]] static macro_cache_key create_from_context(context::hlasm_context& hlasm_ctx, library_data data);
    static void sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state);
    static std::vector<cached_opsyn_mnemo> get_opsyn_state(context::hlasm_context& hlasm_ctx);
    utils::resource::resource_location opencode_file_location;
    library_data data;
    std::vector<cached_opsyn_mnemo> opsyn_state;
};

bool inline operator<(const cached_opsyn_mnemo& lhs, const cached_opsyn_mnemo& rhs)
{
    const static auto tie_cached_opsyn_mnemo = [](const cached_opsyn_mnemo& item) {
        return std::tie(item.from_instr, item.to_instr, item.is_macro);
    };

    return tie_cached_opsyn_mnemo(lhs) < tie_cached_opsyn_mnemo(rhs);
}
bool inline operator==(const cached_opsyn_mnemo& lhs, const cached_opsyn_mnemo& rhs)
{
    return lhs.from_instr == rhs.from_instr && lhs.to_instr == rhs.to_instr && lhs.is_macro == rhs.is_macro;
}

bool inline operator<(const macro_cache_key& lhs, const macro_cache_key& rhs)
{
    const static auto tie_macro_cache_key = [](const macro_cache_key& key) {
        return std::tie(key.data.proc_kind, key.data.library_member, key.opsyn_state);
    };

    return tie_macro_cache_key(lhs) < tie_macro_cache_key(rhs);
}

using version_stamp =
    std::unordered_map<utils::resource::resource_location, version_t, utils::resource::resource_location_hasher>;

// Pair of version stamp and analyzer that parsed the version of file(s)
struct macro_cache_data
{
    // Versions of respective macro (copy member) with all dependencies (COPY instruction is evaluated during macro
    // definition and the statements are part of macro definition)
    version_stamp stamps;
    std::variant<lsp::macro_info_ptr, context::copy_member_ptr> cached_member;
};

class macro_cache final : public diagnosable_impl
{
    std::map<macro_cache_key, macro_cache_data> cache_;
    const file_manager* file_mngr_;
    file* macro_file_;

public:
    macro_cache(const file_manager& file_mngr, file& macro_file);
    // Checks whether any dependencies with specified macro cache key (macro context) have changed. If not, loads the
    // cached macro to the specified context. Returns true, if the macro was loaded.
    bool load_from_cache(const macro_cache_key& key, const analyzing_context& ctx);
    void save_macro(const macro_cache_key& key, const analyzer& analyzer);
    void erase_cache_of_opencode(const utils::resource::resource_location& opencode_file_location);

private:
    [[nodiscard]] const macro_cache_data* find_cached_data(const macro_cache_key& key) const;
    [[nodiscard]] version_stamp get_copy_member_versions(context::macro_def_ptr ctx) const;

    void collect_diags() const override;
};

} // namespace hlasm_plugin::parser_library::workspaces


#endif
