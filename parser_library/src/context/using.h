/*
 * Copyright (c) 2022 Broadcom.
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
#include <vector>

#include "id_index.h"
#include "ordinary_assembly/symbol_dependency_tables.h"
#include "ordinary_assembly/symbol_value.h"
#include "source_context.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
template<typename T>
class diagnostic_consumer_t;
struct diagnostic_op;
struct diagnostic;
class library_info;

namespace expressions {
class mach_expression;
} // namespace expressions

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {

class dependency_solver;
class ordinary_assembly_context;
class section;
class using_context;
struct using_evaluate_result;
struct using_context_description;

class using_collection
{
    using mach_expression = expressions::mach_expression;

    struct failed_entry_resolved;
    struct using_entry_resolved;
    struct drop_entry_resolved;

    struct expression_value;
    struct instruction_context;
    class using_drop_definition;
    struct using_entry;

public:
    using register_t = signed char;
    static constexpr register_t invalid_register = -1;
    using offset_t = int32_t;

    using evaluate_result = using_evaluate_result;

private:
    static constexpr size_t reg_set_size = 16;
    using register_set_t = std::array<register_t, reg_set_size>;
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

        constexpr qualified_address(id_index qualifier, const section* sect, offset_t offset) noexcept
            : qualifier(qualifier)
            , sect(sect)
            , offset(offset)
        {}
    };

    using resolved_entry = std::variant<failed_entry_resolved, using_entry_resolved, drop_entry_resolved>;

    struct failed_entry_resolved
    {
        index_t<using_collection> parent;
    };

    struct using_entry_resolved
    {
        index_t<using_collection> parent;

        id_index label;

        const section* owner = nullptr;
        offset_t begin = 0;
        offset_t length = 0;

        register_set_t reg_set = invalid_register_set;
        offset_t reg_offset = 0;

        using_entry_resolved(index_t<using_collection> parent,
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
            std::ranges::copy(regs, reg_set.begin());
        }
    };

    struct drop_entry_resolved
    {
        index_t<using_collection> parent;
        std::vector<std::pair<std::variant<id_index, register_t>, range>> drop;

        drop_entry_resolved(
            index_t<using_collection> parent, std::vector<std::pair<std::variant<id_index, register_t>, range>> drop)
            : parent(parent)
            , drop(std::move(drop))
        {}
    };


    //<label> USING begin,b                    m_begin != nullptr, m_end == nullptr
    //<label> USING (begin,end),b1,b2,...      m_begin != nullptr, m_end != nullptr
    //        DROP  x                          m_begin, m_end == nullptr, m_base = x
    class using_drop_definition
    {
        index_t<using_collection> m_parent;
        id_index m_label;
        index_t<mach_expression> m_begin;
        index_t<mach_expression> m_end;
        std::vector<index_t<mach_expression>> m_base;

        resolved_entry resolve_using(const using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const;
        resolved_entry resolve_using_dep(const using_collection& coll,
            const std::pair<const section*, offset_t>& b,
            std::optional<offset_t> len,
            const qualified_address& base,
            const range& rng,
            diagnostic_consumer_t<diagnostic_op>& diag) const;
        resolved_entry resolve_drop(const using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const;

        static std::pair<std::optional<qualified_address>, range> abs_or_reloc(
            const using_collection& coll, index_t<mach_expression> e, bool abs_is_register);
        static std::pair<std::variant<std::monostate, qualified_id, using_collection::register_t>, range> reg_or_label(
            const using_collection& coll, index_t<mach_expression> e);

    public:
        friend bool operator==(const using_drop_definition&, const using_drop_definition&) = default;

        using_drop_definition(index_t<using_collection> parent,
            index_t<mach_expression> begin,
            std::vector<index_t<mach_expression>> base,
            id_index label = {},
            index_t<mach_expression> end = {})
            : m_parent(parent)
            , m_label(label)
            , m_begin(begin)
            , m_end(end)
            , m_base(std::move(base))
        {}

        bool is_using() const { return !!m_begin; }
        bool is_drop() const { return !m_begin; }

        resolved_entry resolve(using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const;
    };

    struct context_evaluate_result
    {
        register_set_t mapping_regs;
        offset_t reg_offset;
        offset_t length;

        friend bool operator==(const context_evaluate_result&, const context_evaluate_result&) = default;
    };

    class using_context
    {
        struct entry
        {
            id_index label;
            const section* owner;
            offset_t offset;
            offset_t length;
            register_set_t regs;
            offset_t reg_offset;
        };

        std::vector<entry> m_state;

        context_evaluate_result evaluate(id_index label,
            const section* owner,
            long long offset,
            int32_t min_disp,
            int32_t max_disp,
            bool ignore_length) const;

        context_evaluate_result evaluate(id_index label, const section* owner, offset_t offset, bool long_offset) const;

        using_context() = default;

        friend class using_collection;
    };

    struct using_entry
    {
        using_drop_definition definition;
        resolved_entry resolved;
        using_context context;
        index_t<instruction_context> instruction_ctx;
        index_t<mach_expression> expression_used_limit;

        void resolve(using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag);
        void compute_context(using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag);

        using_entry(index_t<using_collection> parent,
            index_t<mach_expression> expression_used_limit,
            index_t<instruction_context> instruction_ctx,
            index_t<mach_expression> begin,
            std::vector<index_t<mach_expression>> base,
            id_index label,
            index_t<mach_expression> end = {})
            : definition(parent, begin, std::move(base), label, end)
            , instruction_ctx(instruction_ctx)
            , expression_used_limit(expression_used_limit)
        {}
        using_entry(index_t<using_collection> parent,
            index_t<mach_expression> expression_used_limit,
            index_t<instruction_context> instruction_ctx,
            std::vector<index_t<mach_expression>> base)
            : definition(parent, {}, std::move(base))
            , instruction_ctx(instruction_ctx)
            , expression_used_limit(expression_used_limit)
        {}

    private:
        void compute_context_correction(const failed_entry_resolved& f, diagnostic_consumer_t<diagnostic_op>& diag);
        void compute_context_correction(const using_entry_resolved& u, diagnostic_consumer_t<diagnostic_op>& diag);
        void compute_context_correction(const drop_entry_resolved& d, diagnostic_consumer_t<diagnostic_op>& diag);
        size_t compute_context_drop(id_index d);
        size_t compute_context_drop(register_t d);
    };

    struct instruction_context
    {
        dependency_evaluation_context evaluation_ctx;
        processing_stack_t stack;
    };

    struct expression_value
    {
        std::unique_ptr<const mach_expression> expression;
        index_t<instruction_context> context;
        symbol_value value;
        id_index label;
    };

    std::vector<using_entry> m_usings;
    std::vector<expression_value> m_expr_values;
    std::vector<instruction_context> m_instruction_contexts;
    bool m_resolved = false;

    const auto& get(index_t<using_collection> idx) const
    {
        assert(idx);
        return m_usings[idx.value()];
    }

    const auto& get(index_t<mach_expression> idx) const
    {
        assert(idx);
        return m_expr_values[idx.value()];
    }

    const auto& get(index_t<instruction_context> idx) const
    {
        assert(idx);
        return m_instruction_contexts[idx.value()];
    }

    index_t<instruction_context> add(dependency_evaluation_context ctx, processing_stack_t stack);
    index_t<mach_expression> add(std::unique_ptr<const mach_expression> expr, index_t<instruction_context> ctx);

public:
    using_collection() = default;
    using_collection(using_collection&&) noexcept;
    using_collection& operator=(using_collection&&) noexcept;
    ~using_collection();

    void resolve_all(
        ordinary_assembly_context& ord_context, diagnostic_consumer_t<diagnostic>& diag, const library_info& li);

    bool resolved() const { return m_resolved; }

    index_t<using_collection> add(index_t<using_collection> current,
        id_index label,
        std::unique_ptr<mach_expression> begin,
        std::unique_ptr<mach_expression> end,
        std::vector<std::unique_ptr<mach_expression>> arguments,
        dependency_evaluation_context eval_ctx,
        processing_stack_t stack);
    index_t<using_collection> remove(index_t<using_collection> current,
        std::vector<std::unique_ptr<mach_expression>> arguments,
        dependency_evaluation_context eval_ctx,
        processing_stack_t stack);
    index_t<using_collection> remove_all() const { return index_t<using_collection>(); }

    evaluate_result evaluate(index_t<using_collection> context_id,
        id_index label,
        const section* owner,
        offset_t offset,
        bool long_offset) const;

    std::vector<using_context_description> describe(index_t<using_collection> context_id) const;

    bool is_label_mapping_section(index_t<using_collection> context_id, id_index label, const section* owner) const;
};

struct using_evaluate_result
{
    using_collection::register_t reg;
    using_collection::offset_t reg_offset;

    constexpr using_evaluate_result(using_collection::register_t reg, using_collection::offset_t reg_offset) noexcept
        : reg(reg)
        , reg_offset(reg_offset)
    {}

    friend bool operator==(using_evaluate_result, using_evaluate_result) = default;
};

struct using_context_description
{
    id_index label;
    std::optional<id_index> section;
    long offset;
    unsigned long length;
    long reg_offset;
    std::vector<using_collection::register_t> regs;

    bool operator==(const using_context_description&) const noexcept = default;
};

} // namespace hlasm_plugin::parser_library::context

#endif
