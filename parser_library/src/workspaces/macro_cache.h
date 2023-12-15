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

#include <compare>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "context/copy_member.h"
#include "context/id_storage.h"
#include "lsp/macro_info.h"
#include "parse_lib_provider.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library {
class analyzer;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::workspaces {

class file_manager;
class file;

struct cached_opsyn_mnemo
{
    context::id_index from_instr;
    context::id_index to_instr;
    bool is_macro;

    constexpr auto operator<=>(const cached_opsyn_mnemo&) const = default;
};

// Contains all the context that affects parsing an external file (macro or copy member)
struct macro_cache_key
{
    [[nodiscard]] static macro_cache_key create_from_context(context::hlasm_context& hlasm_ctx, library_data data);
    static void sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state);
    static std::vector<cached_opsyn_mnemo> get_opsyn_state(context::hlasm_context& hlasm_ctx);

    library_data data;
    std::vector<cached_opsyn_mnemo> opsyn_state;

    bool operator==(const macro_cache_key&) const = default;
    auto operator<=>(const macro_cache_key& o) const
    {
        if (auto c = data <=> o.data; c != 0)
            return c;
        if (auto c = opsyn_state.size() <=> o.opsyn_state.size(); c != 0)
            return c;
        // libc++ still does not support <=> for vector or lexicographical_compare_three_way
        for (auto l = opsyn_state.begin(), r = o.opsyn_state.begin(); l != opsyn_state.end(); ++l, ++r)
            if (auto c = *l <=> *r; c != 0)
                return c;
        return std::strong_ordering::equal;
    }
};

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

// Macro cache is tied to a specific id_storage
class macro_cache final
{
    std::map<macro_cache_key, macro_cache_data> cache_;
    const file_manager* file_mngr_;
    std::shared_ptr<file> macro_file_;

public:
    macro_cache(const file_manager& file_mngr, std::shared_ptr<file> macro_file);
    // Checks whether any dependencies with specified macro cache key (macro context) have changed. If not, loads the
    // cached macro to the specified context. Returns true, if the macro was loaded.
    std::optional<std::vector<std::shared_ptr<file>>> load_from_cache(
        const macro_cache_key& key, const analyzing_context& ctx) const;
    void save_macro(const macro_cache_key& key, const analyzer& analyzer);

private:
    [[nodiscard]] const macro_cache_data* find_cached_data(const macro_cache_key& key) const;
    [[nodiscard]] version_stamp get_copy_member_versions(context::macro_definition& ctx) const;
};

} // namespace hlasm_plugin::parser_library::workspaces


#endif
