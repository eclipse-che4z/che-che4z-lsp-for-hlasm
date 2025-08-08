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

#include "literal_pool.h"

#include <algorithm>
#include <functional>

#include "checking/diagnostic_collector.h"
#include "context/ordinary_assembly/ordinary_assembly_context.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/postponed_statement.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "diagnosable_ctx.h"
#include "ebcdic_encoding.h"
#include "hlasm_context.h"
#include "instructions/instruction.h"
#include "processing/statement.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::context {

id_index literal_pool::add_literal(const std::string& literal_text,
    const std::shared_ptr<const expressions::data_definition>& dd,
    range r,
    size_t unique_id,
    std::optional<address> loctr,
    bool align_on_halfword)
{
    unique_id = dd->references_loctr ? unique_id : 0;
    if (auto lit = get_literal(m_current_literal_pool_generation, dd, unique_id); !lit.empty())
        return lit;

    auto [it, inserted] = m_literals.try_emplace(
        literal_id { m_current_literal_pool_generation, unique_id, dd }, literal_text, r, std::move(loctr));
    // even if we end up inserting a duplicate
    // we need to try to insert const expressions::data_definition->iterator relation
    // because a single literal may be referenced by independent data_definitions
    m_literals_genmap.try_emplace(literal_id { m_current_literal_pool_generation, unique_id, dd }, it);
    // but we should not try to put logical duplicates on the pending queue
    if (inserted)
    {
        m_pending_literals.emplace_back(it);
        it->second.stack = hlasm_ctx.processing_stack();
    }
    else if (it->second.ca_expr_only)
    {
        m_pending_literals.emplace_back(it);
        it->second.text = literal_text;
        it->second.r = r;
        it->second.loctr = std::move(loctr); // loctr is valid if !inserted
        it->second.stack = hlasm_ctx.processing_stack();
        it->second.ca_expr_only = false;
    }
    it->second.align_on_halfword |= align_on_halfword;

    return id_index(&it->second.text);
}

id_index literal_pool::get_literal(
    size_t generation, const std::shared_ptr<const expressions::data_definition>& dd, size_t unique_id) const
{
    unique_id = dd->references_loctr ? unique_id : 0;
    auto it = m_literals_genmap.find(literal_id { generation, unique_id, dd });
    if (it == m_literals_genmap.end())
        return id_index();
    return id_index(&it->second->second.text);
}

bool literal_pool::defined_for_ca_expr(std::shared_ptr<const expressions::data_definition> dd) const
{
    if (dd->references_loctr)
        return false;

    return m_literals.contains(literal_id { current_generation(), 0, std::move(dd) });
}

void literal_pool::mentioned_in_ca_expr(std::shared_ptr<const expressions::data_definition> dd)
{
    if (dd->references_loctr)
        return;

    m_literals.try_emplace(literal_id { current_generation(), 0, std::move(dd) }, ca_only_literal());
}

class literal_pool::literal_postponed_statement final : public context::postponed_statement,
                                                        public processing::resolved_statement
{
    semantics::operands_si op;

    static const processing::op_code op_code;
    static const semantics::remarks_si empty_remarks;
    static const semantics::label_si empty_label;
    static const semantics::instruction_si empty_instr;
    static const processing::processing_format dc_format;

public:
    literal_postponed_statement(
        const std::shared_ptr<const expressions::data_definition>& dd, const literal_pool::literal_details& details)
        : context::postponed_statement(details.stack, this)
        , op(details.r, {})
    {
        op.value.push_back(std::make_unique<semantics::data_def_operand_shared>(dd, details.r));
    }

    const range& stmt_range_ref() const override { return op.field_range; }
    const processing::op_code& opcode_ref() const override { return op_code; }
    processing::processing_format format_ref() const override { return dc_format; }
    const semantics::operands_si& operands_ref() const override { return op; }
    std::span<const semantics::literal_si> literals() const override { return {}; }
    const semantics::remarks_si& remarks_ref() const override { return empty_remarks; }
    const semantics::label_si& label_ref() const override { return empty_label; }
    const semantics::instruction_si& instruction_ref() const override { return empty_instr; }
    std::span<const diagnostic_op> diagnostics() const override { return {}; };
};

const processing::op_code literal_pool::literal_postponed_statement::op_code(
    context::id_index("DC"), &instructions::get_assembler_instructions("DC"));
const semantics::remarks_si literal_pool::literal_postponed_statement::empty_remarks({});
const semantics::label_si literal_pool::literal_postponed_statement::empty_label(range {});
const semantics::instruction_si literal_pool::literal_postponed_statement::empty_instr(range {});
const processing::processing_format literal_pool::literal_postponed_statement::dc_format(
    processing::processing_kind::ORDINARY, processing::processing_form::DAT, processing::operand_occurrence::PRESENT);

void literal_pool::generate_pool(diagnosable_ctx& diags, index_t<using_collection> active_using, const library_info& li)
{
    ordinary_assembly_context& ord_ctx = hlasm_ctx.ord_ctx;

    if (m_pending_literals.empty())
        return;

    for (auto& [it, size, alignment] : m_pending_literals)
    {
        const auto& lit = it->first.lit;
        if (!lit->access_data_def_type()) // unknown type
            continue;

        ordinary_assembly_dependency_solver solver(ord_ctx,
            dependency_evaluation_context {
                it->second.loctr,
                it->first.generation,
                it->first.unique_id,
                active_using,
                ord_ctx.current_opcode_generation(),
            },
            li);
        auto bit_length = lit->evaluate_total_length(solver, checking::data_instr_type::DC, diags);
        if (bit_length < 0)
            continue;
        size = (bit_length + 7) / 8;
        if (size == 0)
            continue;

        auto top_alignment = size | 16; // 16B length alignment is the top
        alignment = (~top_alignment & top_alignment - 1) + 1;
    }

    std::ranges::stable_sort(m_pending_literals, std::ranges::greater(), &pending_literal::alignment);

    constexpr auto sectalign = doubleword;
    ord_ctx.align(sectalign, li);

    for (const auto& [it, size, alignment] : m_pending_literals)
    {
        const auto& lit_key = it->first;
        const auto& lit = lit_key.lit;
        const auto& lit_val = it->second;

        ordinary_assembly_dependency_solver solver(ord_ctx,
            dependency_evaluation_context {
                lit_val.loctr,
                lit_key.generation,
                lit_key.unique_id,
                active_using,
                ord_ctx.current_opcode_generation(),
            },
            li);

        if (!lit->access_data_def_type()) // unknown type
            continue;

        // TODO: warn on align > sectalign

        (void)ord_ctx.create_symbol(id_index(&lit_val.text),
            ord_ctx.align(lit_val.align_on_halfword ? halfword : no_align, li),
            symbol_attributes(symbol_origin::DAT,
                ebcdic_encoding::to_ebcdic((unsigned char)lit->get_type_attribute()),
                lit->get_length_attribute(solver, diags),
                lit->get_scale_attribute(solver, diags),
                lit->get_integer_attribute()));

        if (size == 0)
        {
            diags.add_diagnostic(diagnostic_op::error_D031(it->second.r));
            continue;
        }

        ord_ctx.reserve_storage_area(size, no_align, li);

        ord_ctx.symbol_dependencies().add_postponed_statement(
            std::make_unique<literal_postponed_statement>(lit, lit_val),
            dependency_evaluation_context {
                lit_val.loctr,
                lit_key.generation,
                lit_key.unique_id,
                active_using,
                ord_ctx.current_opcode_generation(),
            });
    }

    m_pending_literals.clear();
    ++m_current_literal_pool_generation;
}

bool literal_pool::literal_id::is_similar(const literal_id& ld) const noexcept
{
    return generation == ld.generation && unique_id == ld.unique_id && utils::is_similar(lit, ld.lit);
}

size_t literal_pool::literal_definition_hasher::operator()(const literal_id& ld) const noexcept
{
    return ld.lit->hash() ^ ld.generation ^ ld.unique_id;
}

} // namespace hlasm_plugin::parser_library::context
