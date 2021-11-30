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

#include "context/ordinary_assembly/ordinary_assembly_context.h"
#include "context/ordinary_assembly/postponed_statement.h"
#include "ebcdic_encoding.h"
#include "hlasm_context.h"
#include "processing/statement.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::context {

id_index literal_pool::add_literal(const std::string& literal_text,
    const std::shared_ptr<const expressions::data_definition>& dd,
    range r,
    size_t unique_id,
    std::optional<address> loctr)
{
    unique_id = dd->references_loctr ? unique_id : 0;
    if (auto lit = get_literal(m_current_literal_pool_generation, dd, unique_id))
        return lit;

    // TODO: processing stack
    // TODO: better uniqueness detection of dd
    auto [it, inserted] = m_literals.emplace(literal_definition {
        literal_text, m_current_literal_pool_generation, unique_id, dd, std::move(r), {}, std::move(loctr) });
    // even if we end up inserting a duplicate
    // we need to try to insert const expressions::data_definition->iterator relation
    // because a single literal may be referenced by independent data_definitions
    m_literals_genmap.try_emplace(literal_id { m_current_literal_pool_generation, unique_id, dd }, it);
    // but we should not try to put logical duplicates on the pending queue
    if (inserted)
        m_pending_literals.emplace_back(&*it);

    return &it->text;
}

id_index literal_pool::get_literal(
    size_t generation, const std::shared_ptr<const expressions::data_definition>& dd, size_t unique_id) const
{
    unique_id = dd->references_loctr ? unique_id : 0;
    auto it = m_literals_genmap.find(literal_id { generation, unique_id, dd });
    if (it == m_literals_genmap.end())
        return nullptr;
    return &it->second->text;
}

void literal_pool::generate_pool(hlasm_context& hlasm_ctx, dependency_solver& solver, diagnostic_op_consumer& diags)
{
    ordinary_assembly_context& ord_ctx = hlasm_ctx.ord_ctx;

    if (m_pending_literals.empty())
        return;

    for (auto& [lit, size, alignment] : m_pending_literals)
    {
        if (!lit->value->access_data_def_type()) // unknown type
            continue;

        size = (semantics::data_def_operand::get_operand_value(*lit->value, solver).get_length() + 7) / 8;
        if (size == 0)
            continue;

        auto top_alignment = size | 16; // 16B length alignment is the top
        alignment = (~top_alignment & top_alignment - 1) + 1;
    }

    std::stable_sort(m_pending_literals.begin(), m_pending_literals.end(), [](const auto& l, const auto& r) {
        return l.alignment > r.alignment;
    });

    constexpr auto sectalign = doubleword;
    ord_ctx.align(sectalign);

    for (const auto& [lit, size, alignment] : m_pending_literals)
    {
        // TODO: warn on align > sectalign
        if (size == 0)
            break;

        bool cycle_ok = ord_ctx.create_symbol(&lit->text,
            ord_ctx.align(no_align),
            symbol_attributes(symbol_origin::DAT,
                ebcdic_encoding::a2e[(unsigned char)lit->value->get_type_attribute()],
                lit->value->get_length_attribute(solver)),
            {});

        ord_ctx.reserve_storage_area(size, no_align);

        if (!cycle_ok)
            diags.add_diagnostic(diagnostic_op::error_E033(lit->r));
        else if (lit->value->get_dependencies(solver).contains_dependencies())
        {
            struct literal_postponed_statement : public context::postponed_statement,
                                                 public processing::resolved_statement
            {
                const literal_pool::literal_definition* lit;
                semantics::operands_si op;
                processing::op_code op_code;

            public:
                literal_postponed_statement(const literal_pool::literal_definition* lit, id_storage& ids)
                    : lit(lit)
                    , op(lit->r, {})
                    , op_code(ids.add("DC"), instruction_type::ASM)
                {
                    op.value.push_back(std::make_unique<semantics::data_def_operand>(lit->value, lit->r));
                }

                const processing_stack_t& location_stack() const override { return lit->stack; }

                const processing::resolved_statement* resolved_stmt() const override { return this; }

                const processing::op_code& opcode_ref() const override { return op_code; }
                processing::processing_format format_ref() const override
                {
                    return processing::processing_format(processing::processing_kind::ORDINARY,
                        processing::processing_form::ASM,
                        processing::operand_occurence::PRESENT);
                }
                const semantics::operands_si& operands_ref() const override { return op; }
                const semantics::remarks_si& remarks_ref() const override
                {
                    static const semantics::remarks_si empty_remarks({}, {});
                    return empty_remarks;
                }
                const range& stmt_range_ref() const override { return lit->r; }
                const semantics::label_si& label_ref() const override
                {
                    static const semantics::label_si empty_label(range {});
                    return empty_label;
                }
                const semantics::instruction_si& instruction_ref() const override
                {
                    static const semantics::instruction_si empty_instr(range {});
                    return empty_instr;
                }
                std::pair<const diagnostic_op*, const diagnostic_op*> diagnostics() const override { return {}; };
            };

            auto adder = ord_ctx.symbol_dependencies.add_dependencies(
                std::make_unique<literal_postponed_statement>(lit, hlasm_ctx.ids()),
                { lit->loctr, lit->generation, lit->unique_id });
            adder.add_dependency();
            adder.finish();
        }
    }

    m_pending_literals.clear();
    ++m_current_literal_pool_generation;
}

bool literal_pool::literal_definition::is_similar(const literal_definition& ld) const noexcept
{
    return generation == ld.generation && unique_id == ld.unique_id && utils::is_similar(value, ld.value);
}

size_t literal_pool::literal_definition_hasher::operator()(const literal_definition& ld) const noexcept
{
    return ld.value->hash() ^ ld.generation ^ ld.unique_id;
}

} // namespace hlasm_plugin::parser_library::context
