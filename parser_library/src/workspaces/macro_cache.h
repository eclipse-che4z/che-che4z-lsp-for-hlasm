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

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "context/copy_member.h"
#include "context/id_index.h"
#include "lsp/macro_info.h"
#include "protocol.h"
#include "utils/general_hashers.h"

namespace hlasm_plugin::parser_library {
class analyzer;
struct analyzing_context;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context
namespace hlasm_plugin::parser_library::workspaces {
struct cached_opsyn_mnemo;
struct macro_cache_key;
} // namespace hlasm_plugin::parser_library::workspaces

template<>
struct std::hash<hlasm_plugin::parser_library::workspaces::cached_opsyn_mnemo>
{
    std::size_t operator()(const hlasm_plugin::parser_library::workspaces::cached_opsyn_mnemo& e) const noexcept;
};

template<>
struct std::hash<hlasm_plugin::parser_library::workspaces::macro_cache_key>
{
    std::size_t operator()(const hlasm_plugin::parser_library::workspaces::macro_cache_key& key) const noexcept;
};


namespace hlasm_plugin::parser_library::workspaces {

class file_manager;
class file;

struct cached_opsyn_mnemo
{
    context::id_index from_instr;
    context::id_index to_instr;
    bool is_macro;

    constexpr bool operator==(const cached_opsyn_mnemo&) const noexcept = default;

    std::size_t hash() const noexcept
    {
        using utils::hashers::hash_combine;
        const auto f = std::hash<context::id_index>()(from_instr);
        const auto t = std::hash<context::id_index>()(to_instr);
        return hash_combine(hash_combine(t, f), is_macro);
    }
};

// Contains all the context that affects parsing an external file (macro or copy member)
struct macro_cache_key
{
    [[nodiscard]] static macro_cache_key create_from_context(
        context::hlasm_context& hlasm_ctx, processing::processing_kind kind, context::id_index name);
    static void sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state);
    static std::vector<cached_opsyn_mnemo> get_opsyn_state(context::hlasm_context& hlasm_ctx);

    processing::processing_kind kind;
    context::id_index name;
    std::vector<cached_opsyn_mnemo> opsyn_state;

    bool operator==(const macro_cache_key&) const = default;

    size_t hash() const noexcept
    {
        using utils::hashers::hash_combine;
        auto h = hash_combine(std::hash<context::id_index>()(name), (size_t)kind);

        for (const auto& e : opsyn_state)
            h = hash_combine(h, std::hash<cached_opsyn_mnemo>()(e));

        return h;
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
    std::unordered_map<macro_cache_key, macro_cache_data> cache_;
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

inline std::size_t std::hash<hlasm_plugin::parser_library::workspaces::cached_opsyn_mnemo>::operator()(
    const hlasm_plugin::parser_library::workspaces::cached_opsyn_mnemo& e) const noexcept
{
    return e.hash();
}

inline std::size_t std::hash<hlasm_plugin::parser_library::workspaces::macro_cache_key>::operator()(
    const hlasm_plugin::parser_library::workspaces::macro_cache_key& key) const noexcept
{
    return key.hash();
}

#endif
