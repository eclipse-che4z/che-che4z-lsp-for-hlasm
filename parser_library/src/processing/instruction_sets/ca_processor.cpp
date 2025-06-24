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

#include "ca_processor.h"

#include <stdexcept>
#include <utility>

#include "aread_time.h"
#include "context/hlasm_context.h"
#include "context/variables/set_symbol.h"
#include "context/well_known.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "processing/branching_provider.h"
#include "processing/handler_map.h"
#include "processing/opencode_provider.h"
#include "processing/processing_state_listener.h"
#include "semantics/operand_impls.h"
#include "semantics/operand_visitor.h"
#include "utils/projectors.h"
#include "utils/task.h"
#include "utils/time.h"

using namespace hlasm_plugin::parser_library;
using namespace processing;

namespace {
template<void (ca_processor::*ptr)(const resolved_statement&)>
constexpr auto fn = +[](ca_processor* self, const resolved_statement& stmt) { (self->*ptr)(stmt); };
} // namespace

struct ca_processor::handler_table
{
    using wk = context::well_known;
    using callback = void(ca_processor*, const processing::resolved_statement&);
    static constexpr auto value = make_handler_map<callback>({
        { wk::SETA, fn<&ca_processor::process_SET<context::A_t>> },
        { wk::SETB, fn<&ca_processor::process_SET<context::B_t>> },
        { wk::SETC, fn<&ca_processor::process_SET<context::C_t>> },
        { wk::LCLA, fn<&ca_processor::process_GBL_LCL<context::A_t, false>> },
        { wk::LCLB, fn<&ca_processor::process_GBL_LCL<context::B_t, false>> },
        { wk::LCLC, fn<&ca_processor::process_GBL_LCL<context::C_t, false>> },
        { wk::GBLA, fn<&ca_processor::process_GBL_LCL<context::A_t, true>> },
        { wk::GBLB, fn<&ca_processor::process_GBL_LCL<context::B_t, true>> },
        { wk::GBLC, fn<&ca_processor::process_GBL_LCL<context::C_t, true>> },
        { wk::ANOP, fn<&ca_processor::process_ANOP> },
        { wk::ACTR, fn<&ca_processor::process_ACTR> },
        { wk::AGO, fn<&ca_processor::process_AGO> },
        { wk::AIF, fn<&ca_processor::process_AIF> },
        { context::id_index(), fn<&ca_processor::process_empty> },
        { wk::MACRO, fn<&ca_processor::process_MACRO> },
        { wk::MEND, fn<&ca_processor::process_MEND> },
        { wk::MEXIT, fn<&ca_processor::process_MEXIT> },
        { wk::AREAD, fn<&ca_processor::process_AREAD> },
        { wk::ASPACE, fn<&ca_processor::process_ASPACE> },
        { wk::AEJECT, fn<&ca_processor::process_AEJECT> },
        { wk::MHELP, fn<&ca_processor::process_MHELP> },
    });

    static constexpr auto find(context::id_index id) noexcept { return value.find(id); }
};

ca_processor::ca_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    processing_state_listener& listener,
    opencode_provider& open_code,
    diagnosable_ctx& diag_ctx)
    : instruction_processor(ctx, branch_provider, lib_provider, diag_ctx)
    , listener_(listener)
    , open_code_(&open_code)
{}

void ca_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    register_literals(*stmt, context::no_align, hlasm_ctx.ord_ctx.next_unique_id());

    if (const auto handler = handler_table::find(stmt->opcode_ref().value))
        handler(this, *stmt);
    else
        throw std::out_of_range("ca_processor::handler_table");
}

void ca_processor::register_seq_sym(const processing::resolved_statement& stmt)
{
    if (stmt.label_ref().type == semantics::label_si_type::SEQ)
    {
        const auto& symbol = std::get<semantics::seq_sym>(stmt.label_ref().value);
        branch_provider.register_sequence_symbol(symbol.name, symbol.symbol_range);
    }
    else if (stmt.label_ref().type != semantics::label_si_type::EMPTY)
    {
        add_diagnostic(diagnostic_op::warning_W010("Name field", stmt.label_ref().field_range));
    }
}

template<typename T>
ca_processor::SET_info ca_processor::get_SET_symbol(const processing::resolved_statement& stmt)
{
    if (stmt.label_ref().type != semantics::label_si_type::VAR)
    {
        add_diagnostic(diagnostic_op::error_E014(stmt.label_ref().field_range));
        return {};
    }

    const auto& symbol = std::get<semantics::vs_ptr>(stmt.label_ref().value).get();
    bool is_scalar_expression = symbol->subscript.empty();

    context::A_t index = -1;

    auto name = symbol->evaluate_name(eval_ctx);

    context::set_symbol_base* set_sym = nullptr;

    if (auto var_symbol = get_var_sym(eval_ctx, name); var_symbol)
    {
        if (var_symbol->access_macro_param_base())
        {
            add_diagnostic(diagnostic_op::error_E030("symbolic parameter", symbol->symbol_range));
            return {};
        }

        set_sym = var_symbol->access_set_symbol_base();
        assert(set_sym);
        if (set_sym->type != context::object_traits<T>::type_enum)
        {
            add_diagnostic(diagnostic_op::error_E013("wrong type of variable symbol", symbol->symbol_range));
            return {};
        }
    }
    else
    {
        auto var = hlasm_ctx.create_local_variable<T>(name, is_scalar_expression);
        if (!var)
        {
            add_diagnostic(diagnostic_op::error_E051(name.to_string_view(), symbol->symbol_range));
            return {};
        }

        set_sym = var->access_set_symbol_base();
        assert(set_sym);
    }

    if (symbol->subscript.size() > 1)
    {
        add_diagnostic(diagnostic_op::error_E020("variable symbol subscript", symbol->symbol_range));
        return {};
    }

    if (!is_scalar_expression)
    {
        index = symbol->subscript.front()->evaluate<context::A_t>(eval_ctx);

        if (index < 1)
        {
            add_diagnostic(diagnostic_op::error_E012("subscript value has to be 1 or more", symbol->symbol_range));
            return {};
        }
    }

    if (set_sym->is_scalar ^ is_scalar_expression)
    {
        add_diagnostic(diagnostic_op::error_E013("subscript error", symbol->symbol_range));
        return {};
    }

    return SET_info { set_sym, name, index };
}

template ca_processor::SET_info ca_processor::get_SET_symbol<context::A_t>(const processing::resolved_statement& stmt);
template ca_processor::SET_info ca_processor::get_SET_symbol<context::B_t>(const processing::resolved_statement& stmt);
template ca_processor::SET_info ca_processor::get_SET_symbol<context::C_t>(const processing::resolved_statement& stmt);

bool ca_processor::prepare_SET_operands(
    const processing::resolved_statement& stmt, std::vector<expressions::ca_expression*>& expr_values)
{
    const auto& ops = stmt.operands_ref().value;
    if (ops.empty())
    {
        add_diagnostic(diagnostic_op::error_E022("SET instruction", stmt.instruction_ref().field_range));
        return false;
    }

    for (const auto& op : ops)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            expr_values.push_back(nullptr);
            continue;
        }

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind != semantics::ca_kind::VAR && ca_op->kind != semantics::ca_kind::EXPR)
        {
            add_diagnostic(diagnostic_op::error_E012("SET instruction", ca_op->operand_range));
            return false;
        }

        expr_values.push_back(ca_op->access_expr()->expression.get());
    }
    return true;
}

bool ca_processor::prepare_GBL_LCL(const processing::resolved_statement& stmt, std::vector<GLB_LCL_info>& info) const
{
    bool has_operand = false;
    const auto& ops = stmt.operands_ref().value;
    info.reserve(ops.size());
    for (const auto& op : ops)
    {
        if (op->type == semantics::operand_type::EMPTY)
            continue;

        has_operand = true;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind == semantics::ca_kind::VAR)
        {
            auto [id, subscript] = ca_op->access_var()->variable_symbol->evaluate_symbol(eval_ctx);

            if (id.empty())
                continue;

            if (auto var_sym = get_var_sym(eval_ctx, id))
            {
                if (var_sym->access_set_symbol_base())
                    add_diagnostic(diagnostic_op::error_E051(id.to_string_view(), ca_op->operand_range));
                else if (var_sym->access_macro_param_base())
                    add_diagnostic(diagnostic_op::error_E052(id.to_string_view(), ca_op->operand_range));
                else
                    assert(false);
                continue;
            }

            if (std::ranges::find(info, id, &GLB_LCL_info::id) != info.end())
                add_diagnostic(diagnostic_op::error_E051(id.to_string_view(), ca_op->operand_range));
            else
                info.emplace_back(id, subscript.empty(), ca_op->operand_range);
        }
        else
        {
            add_diagnostic(diagnostic_op::error_E014(ca_op->operand_range));
            return false;
        }
    }

    if (!has_operand)
    {
        add_diagnostic(diagnostic_op::error_E022("variable symbol definition", stmt.instruction_ref().field_range));
        return false;
    }

    return true;
}

void ca_processor::process_ANOP(const processing::resolved_statement& stmt)
{
    assert(stmt.operands_ref().value.empty());
    register_seq_sym(stmt);
}

void ca_processor::process_ACTR(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);

    if (stmt.operands_ref().value.size() != 1)
    {
        add_diagnostic(diagnostic_op::error_E020("operand", stmt.instruction_ref().field_range));
        return;
    }

    const auto* ca_op = stmt.operands_ref().value[0]->access_ca();
    assert(ca_op);

    if (ca_op->kind != semantics::ca_kind::EXPR)
    {
        static constexpr std::string_view expected[] = { "arithmetic expression" };
        add_diagnostic(diagnostic_op::error_E015(expected, ca_op->operand_range));
        return;
    }

    const auto ctr = ca_op->access_expr()->expression->evaluate<context::A_t>(eval_ctx);

    static constexpr size_t ACTR_LIMIT = 1000;

    if (hlasm_ctx.set_branch_counter(ctr) == ACTR_LIMIT)
    {
        add_diagnostic(diagnostic_op::error_W063(stmt.stmt_range_ref()));
    }
}

const semantics::seq_sym* ca_processor::prepare_AGO(const processing::resolved_statement& stmt)
{
    const auto& ops = stmt.operands_ref().value;
    if (ops.empty())
    {
        add_diagnostic(diagnostic_op::error_E022("AGO", stmt.instruction_ref().field_range));
        return nullptr;
    }

    static constexpr std::string_view expected[] = { "sequence symbol" };
    for (const auto& op : ops)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            add_diagnostic(diagnostic_op::error_E015(expected, op->operand_range));
            return nullptr;
        }
    }

    auto ca_op = ops[0]->access_ca();
    assert(ca_op);

    if (ca_op->kind == semantics::ca_kind::SEQ)
    {
        if (ops.size() != 1)
        {
            add_diagnostic(diagnostic_op::error_E015(expected, ca_op->operand_range));
            return nullptr;
        }

        return &ca_op->access_seq()->sequence_symbol;
    }

    if (ca_op->kind == semantics::ca_kind::BRANCH)
    {
        const semantics::seq_sym* result = nullptr;

        auto br_op = ca_op->access_branch();
        auto branch = br_op->expression->evaluate<context::A_t>(eval_ctx);
        if (branch == 1)
            result = &br_op->sequence_symbol;

        for (size_t i = 1; i < ops.size(); ++i)
        {
            auto tmp = ops[i]->access_ca();
            assert(tmp);

            if (tmp->kind != semantics::ca_kind::SEQ)
            {
                add_diagnostic(diagnostic_op::error_E015(expected, tmp->operand_range));
                return nullptr;
            }

            if (std::cmp_equal(i + 1, branch))
                result = &tmp->access_seq()->sequence_symbol;
        }
        return result;
    }
    return nullptr;
}

void ca_processor::process_AGO(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);

    if (const auto target = prepare_AGO(stmt))
        branch_provider.jump_in_statements(target->name, target->symbol_range);
}

const semantics::seq_sym* ca_processor::prepare_AIF(const processing::resolved_statement& stmt)
{
    const auto& ops = stmt.operands_ref().value;
    if (ops.empty())
    {
        add_diagnostic(diagnostic_op::error_E022("AIF", stmt.instruction_ref().field_range));
        return nullptr;
    }

    static constexpr std::string_view expected[] = { "conditional branch" };
    bool has_operand = false;
    const semantics::seq_sym* result = nullptr;
    for (auto it = ops.begin(); it != ops.end(); ++it)
    {
        const auto& op = *it;

        if (op->type == semantics::operand_type::EMPTY)
        {
            if (it == ops.end() - 1)
                continue;

            add_diagnostic(diagnostic_op::error_E015(expected, op->operand_range));
            return nullptr;
        }
        has_operand = true;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind != semantics::ca_kind::BRANCH)
        {
            add_diagnostic(diagnostic_op::error_E015(expected, ca_op->operand_range));
            return nullptr;
        }

        if (result)
            continue;

        if (const auto* br = ca_op->access_branch(); br->expression->evaluate<context::B_t>(eval_ctx))
            result = &br->sequence_symbol;
    }

    if (!has_operand)
    {
        add_diagnostic(diagnostic_op::error_E022("variable symbol definition", stmt.instruction_ref().field_range));
        return nullptr;
    }

    return result;
}

void ca_processor::process_AIF(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);

    if (const auto target = prepare_AIF(stmt))
        branch_provider.jump_in_statements(target->name, target->symbol_range);
}

void ca_processor::process_MACRO(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);
    listener_.start_macro_definition({});
}

void ca_processor::process_MEXIT(const processing::resolved_statement& stmt)
{
    if (!hlasm_ctx.is_in_macro())
        add_diagnostic(diagnostic_op::error_E054(stmt.stmt_range_ref()));
    else
        hlasm_ctx.leave_macro();
}

void ca_processor::process_MEND(const processing::resolved_statement& stmt)
{
    if (!hlasm_ctx.is_in_macro())
        add_diagnostic(diagnostic_op::error_E054(stmt.stmt_range_ref()));
}

void ca_processor::process_AEJECT(const processing::resolved_statement&)
{
    // TODO
}

void ca_processor::process_ASPACE(const processing::resolved_statement& stmt)
{
    // TODO
    (void)stmt;
}

namespace {
struct AREAD_operand_visitor final : public semantics::operand_visitor
{
    explicit AREAD_operand_visitor(expressions::evaluation_context* ectx)
        : eval_ctx(ectx)
    {}

    expressions::evaluation_context* eval_ctx = nullptr;
    context::id_index value;

    void visit(const semantics::empty_operand&) override {}
    void visit(const semantics::model_operand&) override {}
    void visit(const semantics::expr_machine_operand&) override {}
    void visit(const semantics::address_machine_operand&) override {}
    void visit(const semantics::expr_assembler_operand&) override {}
    void visit(const semantics::using_instr_assembler_operand&) override {}
    void visit(const semantics::complex_assembler_operand&) override {}
    void visit(const semantics::string_assembler_operand&) override {}
    void visit(const semantics::data_def_operand&) override {}
    void visit(const semantics::var_ca_operand&) override {}
    void visit(const semantics::expr_ca_operand& op) override
    {
        if (const auto* symbol = dynamic_cast<const expressions::ca_symbol*>(op.expression.get()))
            value = symbol->symbol;
    }
    void visit(const semantics::seq_ca_operand&) override {}
    void visit(const semantics::branch_ca_operand&) override {}
    void visit(const semantics::macro_operand&) override {}
};
} // namespace

void ca_processor::process_AREAD(const processing::resolved_statement& stmt)
{
    if (stmt.label_ref().type != semantics::label_si_type::VAR)
    {
        add_diagnostic(diagnostic_op::error_E014(stmt.label_ref().field_range));
        return;
    }
    if (!hlasm_ctx.current_scope().is_in_macro())
    {
        add_diagnostic(diagnostic_op::error_E069(stmt.instruction_ref().field_range));
        return;
    }

    auto& ops = stmt.operands_ref();
    enum class aread_variant
    {
        invalid,
        reader,
        clockb,
        clockd,
    } const variant = [&eval_ctx = eval_ctx, &ops]() {
        if (ops.value.size() == 0)
            return aread_variant::reader;
        if (ops.value.size() == 2 && ops.value[0]->type == semantics::operand_type::EMPTY
            && ops.value[1]->type == semantics::operand_type::EMPTY)
            return aread_variant::reader;

        AREAD_operand_visitor op(&eval_ctx);
        ops.value.at(0)->apply(op);

        if (op.value.empty())
            return aread_variant::invalid;

        static const std::initializer_list<std::pair<std::string_view, aread_variant>> allowed_operands = {
            { "NOSTMT", aread_variant::reader },
            { "NOPRINT", aread_variant::reader },
            { "CLOCKB", aread_variant::clockb },
            { "CLOCKD", aread_variant::clockd },
        };
        auto idx = std::ranges::find(allowed_operands, op.value.to_string_view(), utils::first_element);
        if (idx == allowed_operands.end())
            return aread_variant::invalid;
        return idx->second;
    }();

    if (variant == aread_variant::invalid)
    {
        add_diagnostic(diagnostic_op::error_E070(ops.field_range));
        return;
    }

    auto [set_symbol, name, index] = get_SET_symbol<context::C_t>(stmt);

    if (!set_symbol)
        return;

    constexpr auto since_midnight = []() -> std::chrono::nanoseconds {
        using namespace std::chrono;

        const auto now = utils::timestamp::now().value_or(utils::timestamp());

        return hours(now.hour()) + minutes(now.minute()) + seconds(now.second()) + microseconds(now.microsecond());
    };

    std::string value_to_set;
    switch (variant)
    {
        case aread_variant::reader:
            if (auto aread_result = open_code_->aread(); std::holds_alternative<std::string>(aread_result))
                value_to_set = std::move(std::get<std::string>(aread_result));
            else
            {
                listener_.schedule_helper_task([](utils::value_task<std::string> t,
                                                   context::set_symbol_base* set_sym,
                                                   context::A_t idx) -> utils::task {
                    auto value = co_await std::move(t);
                    set_sym->access_set_symbol<context::C_t>()->set_value(std::move(value), idx);
                }(std::move(std::get<utils::value_task<std::string>>(aread_result)), set_symbol, index));
                return;
            }
            break;

        case aread_variant::clockb:
            value_to_set = time_to_clockb(since_midnight());
            break;

        case aread_variant::clockd:
            value_to_set = time_to_clockd(since_midnight());
            break;
    }
    set_symbol->access_set_symbol<context::C_t>()->set_value(std::move(value_to_set), index);
}

void ca_processor::process_empty(const processing::resolved_statement&) {}

template<typename T>
void ca_processor::process_SET(const processing::resolved_statement& stmt)
{
    auto [set_symbol, name, index] = get_SET_symbol<T>(stmt);

    if (!set_symbol)
        return;

    m_set_work.clear();
    if (!prepare_SET_operands(stmt, m_set_work))
        return;

    if (m_set_work.size() > std::numeric_limits<context::A_t>::max()
        || std::numeric_limits<context::A_t>::max() - (context::A_t)m_set_work.size() < index)
    {
        eval_ctx.diags.add_diagnostic(diagnostic_op::error_E080(stmt.operands_ref().field_range));
        return;
    }

    for (context::A_t i = 0; i < m_set_work.size(); ++i)
    {
        // first obtain a place to put the result in
        auto& val = set_symbol->template access_set_symbol<T>()->reserve_value(index + i);
        // then evaluate the new value and save it unless the operand is empty
        if (m_set_work[i])
            val = m_set_work[i]->template evaluate<T>(eval_ctx);
    }
}

template void ca_processor::process_SET<context::A_t>(const processing::resolved_statement& stmt);
template void ca_processor::process_SET<context::B_t>(const processing::resolved_statement& stmt);
template void ca_processor::process_SET<context::C_t>(const processing::resolved_statement& stmt);

template<typename T, bool global>
void ca_processor::process_GBL_LCL(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);

    m_glb_lcl_work.clear();
    if (!prepare_GBL_LCL(stmt, m_glb_lcl_work))
        return;

    for (const auto& i : m_glb_lcl_work)
    {
        if constexpr (global)
        {
            if (!hlasm_ctx.create_global_variable<T>(i.id, i.scalar))
                eval_ctx.diags.add_diagnostic(diagnostic_op::error_E078(i.id.to_string_view(), i.r));
        }
        else
        {
            if (!hlasm_ctx.create_local_variable<T>(i.id, i.scalar))
                eval_ctx.diags.add_diagnostic(diagnostic_op::error_E051(i.id.to_string_view(), i.r));
        }
    }
}

template void ca_processor::process_GBL_LCL<context::A_t, false>(const processing::resolved_statement& stmt);
template void ca_processor::process_GBL_LCL<context::B_t, false>(const processing::resolved_statement& stmt);
template void ca_processor::process_GBL_LCL<context::C_t, false>(const processing::resolved_statement& stmt);
template void ca_processor::process_GBL_LCL<context::A_t, true>(const processing::resolved_statement& stmt);
template void ca_processor::process_GBL_LCL<context::B_t, true>(const processing::resolved_statement& stmt);
template void ca_processor::process_GBL_LCL<context::C_t, true>(const processing::resolved_statement& stmt);

void ca_processor::process_MHELP(const processing::resolved_statement& stmt)
{
    register_seq_sym(stmt);

    const auto& ops = stmt.operands_ref().value;
    if (ops.size() > 1)
    {
        add_diagnostic(diagnostic_op::error_E020("operand", stmt.instruction_ref().field_range));
        return;
    }
    if (ops.size() < 1)
    {
        add_diagnostic(diagnostic_op::error_E021("operand", stmt.instruction_ref().field_range));
        return;
    }

    const auto* ca_op = ops[0]->access_ca();
    assert(ca_op);
    if (!ca_op)
        return;

    uint32_t value = 0;
    if (ca_op->kind == semantics::ca_kind::EXPR)
    {
        value = ca_op->access_expr()->expression->evaluate<context::A_t>(eval_ctx);
    }
    else if (ca_op->kind == semantics::ca_kind::VAR)
    {
        auto val = ca_op->access_var()->variable_symbol->evaluate(eval_ctx);
        if (val.type() == context::SET_t_enum::A_TYPE)
            value = val.access_a();
    }
    else
    {
        static constexpr std::string_view expected[] = { "arithmetic expression", "arithmetic variable" };
        add_diagnostic(diagnostic_op::error_E015(expected, ca_op->operand_range));
    }
    value &= ~0xffUL; // ignore the option part
    if (value & 0xff00UL) // rest is considered only when byte 3 is non-zero
        hlasm_ctx.sysndx_limit(std::min((unsigned long)value, context::hlasm_context::sysndx_limit_max()));
}
