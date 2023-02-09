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

template<typename T>
class comparable_weak_ptr
{
    std::weak_ptr<T> ptr;
    T* direct_ptr;

public:
    explicit comparable_weak_ptr(const std::shared_ptr<T>& s)
        : ptr(s)
        , direct_ptr(s.get())
    {}

    bool expired() const { return ptr.expired(); }
    auto lock() const { return ptr.lock(); }

    bool operator==(const comparable_weak_ptr& o) const
    {
        if (ptr.owner_before(o.ptr) || o.ptr.owner_before(ptr))
            return false;
        return std::compare_three_way()(direct_ptr, o.direct_ptr);
    }
    auto operator<=>(const comparable_weak_ptr& o) const
    {
        if (ptr.owner_before(o.ptr))
            return std::strong_ordering::less;
        if (o.ptr.owner_before(ptr))
            return std::strong_ordering::greater;
        return std::compare_three_way()(direct_ptr, o.direct_ptr);
    }
    bool operator==(const std::shared_ptr<T>& o) const
    {
        if (ptr.owner_before(o) || o.owner_before(ptr))
            return false;
        return std::compare_three_way()(direct_ptr, o.get());
    }
    auto operator<=>(const std::shared_ptr<T>& o) const
    {
        if (ptr.owner_before(o))
            return std::strong_ordering::less;
        if (o.owner_before(ptr))
            return std::strong_ordering::greater;
        return std::compare_three_way()(direct_ptr, o.get());
    }
};

// Contains all the context that affects parsing an external file (macro or copy member)
struct macro_cache_key
{
    [[nodiscard]] static macro_cache_key create_from_context(context::hlasm_context& hlasm_ctx, library_data data);
    static void sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state);
    static std::vector<cached_opsyn_mnemo> get_opsyn_state(context::hlasm_context& hlasm_ctx);

    comparable_weak_ptr<context::id_storage> related_open_code;
    library_data data;
    std::vector<cached_opsyn_mnemo> opsyn_state;

    bool operator==(const macro_cache_key&) const = default;
    auto operator<=>(const macro_cache_key& o) const
    {
        if (auto c = related_open_code <=> o.related_open_code; c != 0)
            return c;
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

class macro_cache final
{
    std::map<macro_cache_key, macro_cache_data> cache_;
    const file_manager* file_mngr_;
    std::shared_ptr<file> macro_file_;

public:
    macro_cache(const file_manager& file_mngr, std::shared_ptr<file> macro_file);
    // Checks whether any dependencies with specified macro cache key (macro context) have changed. If not, loads the
    // cached macro to the specified context. Returns true, if the macro was loaded.
    bool load_from_cache(const macro_cache_key& key, const analyzing_context& ctx) const;
    void save_macro(const macro_cache_key& key, const analyzer& analyzer);
    void erase_unused();

private:
    [[nodiscard]] const macro_cache_data* find_cached_data(const macro_cache_key& key) const;
    [[nodiscard]] version_stamp get_copy_member_versions(context::macro_def_ptr ctx) const;
};

} // namespace hlasm_plugin::parser_library::workspaces


#endif
