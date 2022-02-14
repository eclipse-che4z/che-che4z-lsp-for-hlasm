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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_USING_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_USING_H

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>

#include "id_storage.h"
#include "ordinary_assembly/symbol_value.h"

namespace hlasm_plugin::parser_library {
template<typename T>
class diagnostic_consumer;
struct diagnostic_op;

namespace expressions {
class mach_expression;
} // namespace expressions

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {

class using_context;
class section;
class dependency_solver;

class using_collection
{
    using mach_expression = expressions::mach_expression;
    struct using_entry_resolved;
    class using_drop_definition;
    struct using_entry;

public:
    using register_t = unsigned char;
    static constexpr size_t reg_set_size = 16;
    using register_set_t = std::array<register_t, reg_set_size>;
    static constexpr register_t invalid_register = (unsigned char)-1;
    static constexpr register_set_t invalid_register_set = {
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
        invalid_register,
    };
    using offset_t = int32_t;

    struct index_t
    {
        size_t index = (size_t)-1;

        friend auto operator<=>(const index_t& l, const index_t& r) = default;
        explicit operator bool() const { return index != (size_t)-1; }
        const using_entry& operator()(const using_collection* coll)
        {
            assert(*this);
            return coll->m_usings[index];
        }
    };

    struct qualified_id
    {
        id_index qualifier;
        id_index name;
    };

    struct qualified_address
    {
        id_index qualifier;
        const section* sect;
        offset_t offset;
    };

private:
    struct using_entry_resolved
    {
        index_t parent;

        id_index label = nullptr;

        const section* owner = nullptr;
        offset_t begin = 0;
        offset_t length = 0;

        register_set_t reg_set = invalid_register_set;
        offset_t reg_offset = 0;

        constexpr using_entry_resolved(index_t parent,
            id_index label,
            const section* owner,
            offset_t begin,
            offset_t length,
            std::span<register_t> regs,
            offset_t reg_offset)
            : parent(parent)
            , label(label)
            , owner(owner)
            , begin(begin)
            , length(length)
            , reg_offset(reg_offset)
        {
            assert(length >= 0);
            assert(regs.size() <= reg_set.size());
            std::copy(regs.begin(), regs.end(), reg_set.begin());
        }
    };

    struct drop_entry_resolved
    {
        index_t parent;
        std::vector<std::variant<id_index, register_t>> drop;

        constexpr drop_entry_resolved(index_t parent, std::vector<std::variant<id_index, register_t>> drop)
            : parent(parent)
            , drop(std::move(drop))
        {}
    };

    using resolved_entry = std::variant<index_t, using_entry_resolved, drop_entry_resolved>;


    //<label> USING begin,b                    m_begin != nullptr, m_end == nullptr
    //<label> USING (begin,end),b1,b2,...      m_begin != nullptr, m_end != nullptr
    //        DROP  x                          m_begin, m_end == nullptr, m_base = x
    class using_drop_definition
    {
        index_t m_parent;
        id_index m_label;
        const mach_expression* m_begin;
        const mach_expression* m_end;
        std::array<const mach_expression*, reg_set_size> m_base = {};

        resolved_entry resolve_using(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const;
        resolved_entry resolve_using_dep(using_collection& coll,
            const std::pair<const section*, offset_t>& b,
            std::optional<offset_t> len,
            const qualified_address& base,
            diagnostic_consumer<diagnostic_op>& diag) const;
        resolved_entry resolve_drop(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const;

        static std::optional<qualified_address> abs_or_reloc(
            using_collection& coll, const mach_expression* e, bool abs_is_register = false);
        static std::variant<std::monostate, qualified_id, using_collection::register_t> abs_or_label(
            using_collection& coll, const mach_expression* e, bool allow_qualification);

    public:
        friend auto operator<=>(const using_drop_definition&, const using_drop_definition&) = default;

        constexpr using_drop_definition(index_t parent,
            const mach_expression* begin,
            std::span<const mach_expression*> base,
            id_index label = nullptr,
            const mach_expression* end = nullptr)
            : m_parent(parent)
            , m_label(label)
            , m_begin(begin)
            , m_end(end)
        {
            assert(base.size() <= m_base.size());
            std::copy(base.begin(), base.end(), m_base.begin());
        }

        constexpr bool is_using() const { return m_begin != nullptr; }
        constexpr bool is_drop() const { return m_begin == nullptr; }

        resolved_entry resolve(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const;
    };

    class using_context
    {
    public:
        struct evaluate_result
        {
            register_set_t mapping_regs;
            offset_t reg_offset;
            offset_t length;
        };

    private:
        struct entry
        {
            id_index label;
            const section* section;
            offset_t offset;
            offset_t length;
            register_set_t regs;
            offset_t reg_offset;
        };

        std::vector<entry> m_state;

        evaluate_result evaluate(const using_collection& coll,
            id_index label,
            const section* section,
            long long offset,
            int32_t min_disp,
            int32_t max_disp) const;

    public:
        evaluate_result evaluate(const using_collection& coll,
            id_index label,
            const section* section,
            offset_t offset,
            bool long_offset) const;

        constexpr using_context() = default;

        friend class using_collection;
    };

    struct using_entry
    {
        using_drop_definition definition;
        resolved_entry resolved;
        using_context context;

        void resolve(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag);
        void compute_context(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag);

        constexpr using_entry(index_t parent,
            const mach_expression* begin,
            std::span<const mach_expression*> base,
            id_index label = nullptr,
            const mach_expression* end = nullptr)
            : definition(parent, begin, base, label, end)
        {}
        constexpr using_entry(index_t parent, std::span<const mach_expression*> base)
            : definition(parent, nullptr, base)
        {}

    private:
        void duplicate_parent_context(using_collection& coll, index_t parent);
        void compute_context(using_collection& coll, index_t parent, diagnostic_consumer<diagnostic_op>& diag);
        void compute_context(
            using_collection& coll, const using_entry_resolved& u, diagnostic_consumer<diagnostic_op>& diag);
        void compute_context(
            using_collection& coll, const drop_entry_resolved& d, diagnostic_consumer<diagnostic_op>& diag);
        size_t compute_context_drop(id_index d);
        size_t compute_context_drop(register_t d);
    };

    struct expression_hash
    {
        using is_transparent = void;

        size_t operator()(const std::unique_ptr<mach_expression>& v) const;
        size_t operator()(const mach_expression* v) const;
    };

    struct expression_equal
    {
        using is_transparent = void;

        bool operator()(const std::unique_ptr<mach_expression>& l, const std::unique_ptr<mach_expression>& r) const;
        bool operator()(const mach_expression* l, const std::unique_ptr<mach_expression>& r) const;
        bool operator()(const std::unique_ptr<mach_expression>& l, const mach_expression* r) const;
    };

    std::unordered_map<std::unique_ptr<mach_expression>, symbol_value, expression_hash, expression_equal> m_expressions;
    std::vector<using_entry> m_usings;

    const symbol_value& eval_expr(const mach_expression*) const;

public:
    using_collection() = default;
    using_collection(using_collection&&) noexcept;
    using_collection& operator=(using_collection&&) noexcept;
    ~using_collection();

    void resolve_all(dependency_solver& solver, diagnostic_consumer<diagnostic_op>& diag);

    index_t add(index_t current,
        id_index label,
        std::unique_ptr<mach_expression> begin,
        std::unique_ptr<mach_expression> end,
        std::span<std::unique_ptr<mach_expression>> arguments,
        diagnostic_consumer<diagnostic_op>& diag);
    index_t remove(index_t current,
        std::span<std::unique_ptr<mach_expression>> arguments,
        diagnostic_consumer<diagnostic_op>& diag);
    index_t remove_all() const { return index_t(); }
};

} // namespace hlasm_plugin::parser_library::context

#endif
