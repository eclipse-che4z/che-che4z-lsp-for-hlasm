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

#include "operand_impls.h"

#include <limits>

#include "context/instruction.h"
#include "context/ordinary_assembly/section.h"
#include "context/using.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expr_visitor.h"
#include "expressions/mach_operator.h"
#include "operand_visitor.h"

namespace hlasm_plugin::parser_library::semantics {


//***************** operand *********************

operand::operand(const operand_type type, range operand_range)
    : type(type)
    , operand_range(std::move(operand_range))
{}

model_operand* operand::access_model()
{
    return type == operand_type::MODEL ? static_cast<model_operand*>(this) : nullptr;
}

ca_operand* operand::access_ca() { return type == operand_type::CA ? static_cast<ca_operand*>(this) : nullptr; }

macro_operand* operand::access_mac() { return type == operand_type::MAC ? static_cast<macro_operand*>(this) : nullptr; }

data_def_operand* operand::access_data_def()
{
    return type == operand_type::DAT ? static_cast<data_def_operand*>(this) : nullptr;
}

machine_operand* operand::access_mach() { return dynamic_cast<machine_operand*>(this); }

assembler_operand* operand::access_asm() { return dynamic_cast<assembler_operand*>(this); }

//***************** empty, model, evaluable operand *********************

empty_operand::empty_operand(range operand_range)
    : operand(operand_type::EMPTY, std::move(operand_range))
{}

void empty_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

model_operand::model_operand(concat_chain chain, range operand_range)
    : operand(operand_type::MODEL, std::move(operand_range))
    , chain(std::move(chain))
{}

void model_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

evaluable_operand::evaluable_operand(const operand_type type, range operand_range)
    : operand(type, std::move(operand_range))
{}

//***************** machine_operand *********************

machine_operand::machine_operand(const mach_kind kind)
    : kind(kind)
{}

expr_machine_operand* machine_operand::access_expr()
{
    return kind == mach_kind::EXPR ? static_cast<expr_machine_operand*>(this) : nullptr;
}

address_machine_operand* machine_operand::access_address()
{
    return kind == mach_kind::ADDR ? static_cast<address_machine_operand*>(this) : nullptr;
}

std::unique_ptr<checking::operand> make_check_operand(context::dependency_solver& info,
    const expressions::mach_expression& expr,
    const checking::machine_operand_format& mach_op_type,
    diagnostic_op_consumer& diags)
{
    auto res = expr.evaluate(info, diags);
    if (res.value_kind() == context::symbol_value_kind::ABS)
    {
        return std::make_unique<checking::one_operand>(res.get_abs());
    }
    else if (res.value_kind() == context::symbol_value_kind::RELOC
        && mach_op_type.identifier.type == checking::machine_operand_type::DISPLC)
    {
        const auto& reloc = res.get_reloc();
        if (reloc.is_simple())
        {
            const auto& base = reloc.bases().front().first;
            auto translated_addr = info.using_evaluate(
                base.qualifier, base.owner, reloc.offset(), mach_op_type.identifier == context::dis_20s);
            if (translated_addr.reg != context::using_collection::invalid_register)
            {
                // TODO: this does not work correctly for d(L,r) type of operand,
                // we really need the operand type here, to do the right thing.
                // NOTE: length of the leftmost operand determines the value
                return std::make_unique<checking::address_operand>(checking::address_state::UNRES,
                    translated_addr.reg_offset,
                    0,
                    translated_addr.reg,
                    checking::operand_state::ONE_OP);
            }
            else
            {
                if (translated_addr.reg_offset)
                    diags.add_diagnostic(diagnostic_op::error_ME008(translated_addr.reg_offset, expr.get_range()));
                else
                    diags.add_diagnostic(diagnostic_op::error_ME007(expr.get_range()));
            }
        }
        else
            diags.add_diagnostic(diagnostic_op::error_ME009(expr.get_range()));
    }

    // everything was already diagnosed
    return std::make_unique<checking::address_operand>(
        checking::address_state::RES_VALID, 0, 0, 0, checking::operand_state::ONE_OP);
}

std::unique_ptr<checking::operand> make_rel_imm_operand(
    context::dependency_solver& info, const expressions::mach_expression& expr, diagnostic_op_consumer& diags)
{
    auto res = expr.evaluate(info, diags);
    if (res.value_kind() == context::symbol_value_kind::ABS)
    {
        return std::make_unique<checking::one_operand>(std::to_string(res.get_abs()), res.get_abs());
    }
    else
    {
        return std::make_unique<checking::address_operand>(
            checking::address_state::UNRES, 0, 0, 0, checking::operand_state::ONE_OP);
    }
}
//***************** expr_machine_operand *********************

expr_machine_operand::expr_machine_operand(expressions::mach_expr_ptr expression, range operand_range)
    : evaluable_operand(operand_type::MACH, std::move(operand_range))
    , machine_operand(mach_kind::EXPR)
    , simple_expr_operand(std::move(expression))
{}

std::unique_ptr<checking::operand> expr_machine_operand::get_operand_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return make_check_operand(info, *expression, { context::empty, context::empty, context::empty, false }, diags);
}

std::unique_ptr<checking::operand> expr_machine_operand::get_operand_value(context::dependency_solver& info,
    const checking::machine_operand_format& mach_op_type,
    diagnostic_op_consumer& diags) const
{
    if (mach_op_type.identifier.type == checking::machine_operand_type::RELOC_IMM)
    {
        return make_rel_imm_operand(info, *expression, diags);
    }
    return make_check_operand(info, *expression, mach_op_type, diags);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_machine_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return simple_expr_operand::has_dependencies(info, missing_symbols);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_machine_operand::has_error(context::dependency_solver& info) const
{
    return simple_expr_operand::has_error(info);
}

void expr_machine_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void expr_machine_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (expression)
        expression->apply(v);
}

//***************** address_machine_operand *********************

address_machine_operand::address_machine_operand(expressions::mach_expr_ptr displacement,
    expressions::mach_expr_ptr first_par,
    expressions::mach_expr_ptr second_par,
    range operand_range,
    checking::operand_state state)
    : evaluable_operand(operand_type::MACH, std::move(operand_range))
    , machine_operand(mach_kind::ADDR)
    , displacement(std::move(displacement))
    , first_par(std::move(first_par))
    , second_par(std::move(second_par))
    , state(std::move(state))
{}

bool address_machine_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return displacement && displacement->has_dependencies(info, missing_symbols)
        || first_par && first_par->has_dependencies(info, missing_symbols)
        || second_par && second_par->has_dependencies(info, missing_symbols);
}

bool address_machine_operand::has_error(context::dependency_solver& info) const
{
    if (first_par)
    {
        if (second_par)
            return displacement->get_dependencies(info).has_error || first_par->get_dependencies(info).has_error
                || second_par->get_dependencies(info).has_error; // D(B1,B2)
        else
            return displacement->get_dependencies(info).has_error
                || first_par->get_dependencies(info).has_error; // D(B)
    }
    else
        return displacement->get_dependencies(info).has_error || second_par->get_dependencies(info).has_error; // D(,B)
}

std::unique_ptr<checking::operand> address_machine_operand::get_operand_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return get_operand_value(info, { context::empty, context::empty, context::empty, false }, diags);
}

namespace {
std::pair<std::optional<context::symbol_value::abs_value_t>, bool> evaluate_abs_expression(
    const expressions::mach_expr_ptr& expr, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    if (!expr)
        return { std::nullopt, false };

    auto value = expr->evaluate(info, diags);
    if (value.value_kind() == context::symbol_value_kind::ABS)
        return { value.get_abs(), false };

    diags.add_diagnostic(diagnostic_op::error_ME010(expr->get_range()));
    return { std::nullopt, true };
}
} // namespace

std::unique_ptr<checking::operand> address_machine_operand::get_operand_value(context::dependency_solver& info,
    const checking::machine_operand_format& mach_op_format,
    diagnostic_op_consumer& diags) const
{
    std::optional<context::symbol_value::abs_value_t> displ_v;

    auto [first_v, first_err] = evaluate_abs_expression(first_par, info, diags);
    auto [second_v, second_err] = evaluate_abs_expression(second_par, info, diags);

    if (auto displ = displacement->evaluate(info, diags); displ.value_kind() == context::symbol_value_kind::ABS)
        displ_v = displ.get_abs();
    else if (displ.value_kind() == context::symbol_value_kind::RELOC)
    {
        if (mach_op_format.identifier.type != checking::machine_operand_type::DISPLC)
        {
            // only translate when memory-like operand indicated
            displ_v = 0;
        }
        else if (first_par && second_par || second_par && state != checking::operand_state::ONE_OP)
        {
            // <reloc>(X,B) and <reloc>(,B) not allowed
            diags.add_diagnostic(diagnostic_op::error_ME011(operand_range));
        }
        else if (const auto& reloc = displ.get_reloc(); !reloc.is_simple())
        {
            diags.add_diagnostic(diagnostic_op::error_ME009(displacement->get_range()));
        }
        else
        {
            if (state == checking::operand_state::ONE_OP)
                std::swap(first_v, second_v); // <reloc>(,X) -> <reloc>(X,?)

            const auto& base = reloc.bases().front().first;
            const bool long_displacement = mach_op_format.identifier == context::dis_20s;
            auto translated_addr = info.using_evaluate(base.qualifier, base.owner, reloc.offset(), long_displacement);
            if (translated_addr.reg != context::using_collection::invalid_register)
            {
                // TODO: this does not work correctly for d(L,r) type of operand,
                // we really need the operand type here, to do the right thing.
                // NOTE: length of the leftmost operand determines the value
                displ_v = translated_addr.reg_offset;
                second_v = translated_addr.reg;
            }
            else
            {
                if (translated_addr.reg_offset)
                    diags.add_diagnostic(
                        diagnostic_op::error_ME008(translated_addr.reg_offset, displacement->get_range()));
                else
                    diags.add_diagnostic(diagnostic_op::error_ME007(displacement->get_range()));
            }
        }
    }

    if (!displ_v.has_value() || first_err || second_err)
        return std::make_unique<checking::address_operand>(checking::address_state::RES_INVALID, 0, 0, 0, state);

    return std::make_unique<checking::address_operand>(checking::address_state::UNRES,
        *displ_v,
        first_v.value_or(0),
        second_v.value_or(0),
        first_v && second_v ? checking::operand_state::PRESENT : state);
}

void address_machine_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void address_machine_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (displacement)
        displacement->apply(v);
    if (first_par)
        first_par->apply(v);
    if (second_par)
        second_par->apply(v);
}

assembler_operand::assembler_operand(const asm_kind kind)
    : kind(kind)
{}

expr_assembler_operand* assembler_operand::access_expr()
{
    return kind == asm_kind::EXPR ? static_cast<expr_assembler_operand*>(this) : nullptr;
}

using_instr_assembler_operand* assembler_operand::access_base_end()
{
    return kind == asm_kind::BASE_END ? static_cast<using_instr_assembler_operand*>(this) : nullptr;
}

complex_assembler_operand* assembler_operand::access_complex()
{
    return kind == asm_kind::COMPLEX ? static_cast<complex_assembler_operand*>(this) : nullptr;
}

string_assembler_operand* assembler_operand::access_string()
{
    return kind == asm_kind::STRING ? static_cast<string_assembler_operand*>(this) : nullptr;
}

//***************** expr_assembler_operand *********************

expr_assembler_operand::expr_assembler_operand(
    expressions::mach_expr_ptr expression, std::string string_value, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::EXPR)
    , simple_expr_operand(std::move(expression))
    , value_(std::move(string_value))
{}

std::unique_ptr<checking::operand> expr_assembler_operand::get_operand_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return get_operand_value_inner(info, true, diags);
}

std::unique_ptr<checking::operand> expr_assembler_operand::get_operand_value(
    context::dependency_solver& info, bool can_have_ordsym, diagnostic_op_consumer& diags) const
{
    return get_operand_value_inner(info, can_have_ordsym, diags);
}

std::unique_ptr<checking::operand> expr_assembler_operand::get_operand_value_inner(
    context::dependency_solver& info, bool can_have_ordsym, diagnostic_op_consumer& diags) const
{
    if (!can_have_ordsym && dynamic_cast<expressions::mach_expr_symbol*>(expression.get()))
        return std::make_unique<checking::one_operand>(value_);

    auto res = expression->evaluate(info, diags);
    switch (res.value_kind())
    {
        case context::symbol_value_kind::UNDEF:
            return std::make_unique<checking::one_operand>(value_);
        case context::symbol_value_kind::ABS:
            return std::make_unique<checking::one_operand>(value_, res.get_abs());
        case context::symbol_value_kind::RELOC:
            return std::make_unique<checking::one_operand>("0", 0);
        default:
            assert(false);
            return std::make_unique<checking::empty_operand>();
    }
}

// suppress MSVC warning 'inherits via dominance'
bool expr_assembler_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return simple_expr_operand::has_dependencies(info, missing_symbols);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_assembler_operand::has_error(context::dependency_solver& info) const
{
    return simple_expr_operand::has_error(info);
}

void expr_assembler_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void expr_assembler_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (expression)
        expression->apply(v);
}

//***************** end_instr_machine_operand *********************

using_instr_assembler_operand::using_instr_assembler_operand(expressions::mach_expr_ptr base,
    expressions::mach_expr_ptr end,
    std::string base_text,
    std::string end_text,
    range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::BASE_END)
    , base(std::move(base))
    , end(std::move(end))
    , base_text(std::move(base_text))
    , end_text(std::move(end_text))
{}

bool using_instr_assembler_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return base->has_dependencies(info, missing_symbols) || end->has_dependencies(info, missing_symbols);
}

bool using_instr_assembler_operand::has_error(context::dependency_solver& info) const
{
    return base->get_dependencies(info).has_error || end->get_dependencies(info).has_error;
}

std::unique_ptr<checking::operand> using_instr_assembler_operand::get_operand_value(
    context::dependency_solver&, diagnostic_op_consumer&) const
{
    std::vector<std::unique_ptr<checking::asm_operand>> pair;
    // this is just for the form
    pair.push_back(std::make_unique<checking::one_operand>(base_text));
    pair.push_back(std::make_unique<checking::one_operand>(end_text));
    return std::make_unique<checking::complex_operand>("", std::move(pair));
}

void using_instr_assembler_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void using_instr_assembler_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (base)
        base->apply(v);
    if (end)
        end->apply(v);
};

//***************** complex_assempler_operand *********************
complex_assembler_operand::complex_assembler_operand(
    std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, range operand_range)
    : evaluable_operand(operand_type::ASM, operand_range)
    , assembler_operand(asm_kind::COMPLEX)
    , value(std::move(identifier), std::move(values), std::move(operand_range))
{}

bool complex_assembler_operand::has_dependencies(context::dependency_solver&, std::vector<context::id_index>*) const
{
    return false;
}

bool complex_assembler_operand::has_error(context::dependency_solver&) const { return false; }

std::unique_ptr<checking::operand> complex_assembler_operand::get_operand_value(
    context::dependency_solver&, diagnostic_op_consumer&) const
{
    return value.create_operand();
}

void complex_assembler_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void complex_assembler_operand::apply_mach_visitor(expressions::mach_expr_visitor&) const {}

//***************** ca_operand *********************
ca_operand::ca_operand(const ca_kind kind, range operand_range)
    : operand(operand_type::CA, std::move(operand_range))
    , kind(kind)
{}

[[nodiscard]] var_ca_operand* ca_operand::access_var()
{
    return kind == ca_kind::VAR ? static_cast<var_ca_operand*>(this) : nullptr;
}

[[nodiscard]] const var_ca_operand* ca_operand::access_var() const
{
    return kind == ca_kind::VAR ? static_cast<const var_ca_operand*>(this) : nullptr;
}

[[nodiscard]] expr_ca_operand* ca_operand::access_expr()
{
    return kind == ca_kind::EXPR ? static_cast<expr_ca_operand*>(this) : nullptr;
}

[[nodiscard]] const expr_ca_operand* ca_operand::access_expr() const
{
    return kind == ca_kind::EXPR ? static_cast<const expr_ca_operand*>(this) : nullptr;
}

[[nodiscard]] seq_ca_operand* ca_operand::access_seq()
{
    return kind == ca_kind::SEQ ? static_cast<seq_ca_operand*>(this) : nullptr;
}

[[nodiscard]] const seq_ca_operand* ca_operand::access_seq() const
{
    return kind == ca_kind::SEQ ? static_cast<const seq_ca_operand*>(this) : nullptr;
}

[[nodiscard]] branch_ca_operand* ca_operand::access_branch()
{
    return kind == ca_kind::BRANCH ? static_cast<branch_ca_operand*>(this) : nullptr;
}

[[nodiscard]] const branch_ca_operand* ca_operand::access_branch() const
{
    return kind == ca_kind::BRANCH ? static_cast<const branch_ca_operand*>(this) : nullptr;
}

simple_expr_operand::simple_expr_operand(expressions::mach_expr_ptr expression)
    : expression(std::move(expression))
{}


[[nodiscard]] bool simple_expr_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return expression->has_dependencies(info, missing_symbols);
}

[[nodiscard]] bool simple_expr_operand::has_error(context::dependency_solver& info) const
{
    return expression->get_dependencies(info).has_error;
}

var_ca_operand::var_ca_operand(vs_ptr variable_symbol, range operand_range)
    : ca_operand(ca_kind::VAR, std::move(operand_range))
    , variable_symbol(std::move(variable_symbol))
{}

bool var_ca_operand::get_undefined_attributed_symbols(
    std::set<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expressions::ca_var_sym::get_undefined_attributed_symbols_vs(symbols, variable_symbol, eval_ctx);
}

void var_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

expr_ca_operand::expr_ca_operand(expressions::ca_expr_ptr expression, range operand_range)
    : ca_operand(ca_kind::EXPR, std::move(operand_range))
    , expression(std::move(expression))
{}

bool expr_ca_operand::get_undefined_attributed_symbols(
    std::set<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(symbols, eval_ctx);
}

void expr_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

seq_ca_operand::seq_ca_operand(seq_sym sequence_symbol, range operand_range)
    : ca_operand(ca_kind::SEQ, std::move(operand_range))
    , sequence_symbol(std::move(sequence_symbol))
{}

bool seq_ca_operand::get_undefined_attributed_symbols(
    std::set<context::id_index>&, const expressions::evaluation_context&)
{
    return false;
}

void seq_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

branch_ca_operand::branch_ca_operand(seq_sym sequence_symbol, expressions::ca_expr_ptr expression, range operand_range)
    : ca_operand(ca_kind::BRANCH, std::move(operand_range))
    , sequence_symbol(std::move(sequence_symbol))
    , expression(std::move(expression))
{}

bool branch_ca_operand::get_undefined_attributed_symbols(
    std::set<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(symbols, eval_ctx);
}

void branch_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }



macro_operand_chain::macro_operand_chain(concat_chain chain, range operand_range)
    : macro_operand(mac_kind::CHAIN, std::move(operand_range))
    , chain(std::move(chain))
{}

void macro_operand_chain::apply(operand_visitor& visitor) const { visitor.visit(*this); }



data_def_operand::data_def_operand(expressions::data_definition val, range operand_range)
    : evaluable_operand(operand_type::DAT, std::move(operand_range))
    , value(std::make_shared<expressions::data_definition>(std::move(val)))
{}

data_def_operand::data_def_operand(std::shared_ptr<const expressions::data_definition> dd_ptr, range operand_range)
    : evaluable_operand(operand_type::DAT, std::move(operand_range))
    , value(std::move(dd_ptr))
{}


context::dependency_collector data_def_operand::get_length_dependencies(context::dependency_solver& info) const
{
    return value->get_length_dependencies(info);
}

context::dependency_collector data_def_operand::get_dependencies(context::dependency_solver& info) const
{
    return value->get_dependencies(info);
}

bool data_def_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    auto deps = value->get_dependencies(info);
    if (missing_symbols)
        deps.collect_unique_symbolic_dependencies(*missing_symbols);
    return deps.contains_dependencies();
}

bool data_def_operand::has_error(context::dependency_solver& info) const
{
    return value->get_dependencies(info).has_error;
}

template<typename... args>
std::vector<const context::resolvable*> resolvable_list(const args&... expr)
{
    std::vector<const context::resolvable*> list;
    (
        [&](auto&& x) {
            if (x)
                list.push_back(x.get());
        }(expr),
        ...);
    return list;
}

std::unique_ptr<checking::operand> data_def_operand::get_operand_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return std::make_unique<checking::data_definition_operand>(get_operand_value(*value, info, diags));
}

checking::data_definition_operand data_def_operand::get_operand_value(
    const expressions::data_definition& dd, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    checking::data_definition_operand op;

    op.dupl_factor = dd.evaluate_dupl_factor(info, diags);
    op.type.value = dd.type;
    op.type.rng = dd.type_range;
    op.extension.present = dd.extension != '\0';
    op.extension.value = dd.extension;
    op.extension.rng = dd.extension_range;
    op.length = dd.evaluate_length(info, diags);
    op.scale = dd.evaluate_scale(info, diags);
    op.exponent = dd.evaluate_exponent(info, diags);

    op.nominal_value = dd.evaluate_nominal_value(info, diags);

    return op;
}

void data_def_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void data_def_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (value)
        value->apply(v);
}

long long data_def_operand::evaluate_total_length(
    context::dependency_solver& info, checking::data_instr_type checking_rules, diagnostic_op_consumer& diags) const
{
    return value->evaluate_total_length(info, checking_rules, diags);
}

string_assembler_operand::string_assembler_operand(std::string value, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::STRING)
    , value(std::move(value))
{}

bool string_assembler_operand::has_dependencies(context::dependency_solver&, std::vector<context::id_index>*) const
{
    return false;
}

bool string_assembler_operand::has_error(context::dependency_solver&) const { return false; }

std::unique_ptr<checking::operand> string_assembler_operand::get_operand_value(
    context::dependency_solver&, diagnostic_op_consumer&) const
{
    return std::make_unique<checking::one_operand>("'" + value + "'");
}

void string_assembler_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }
void string_assembler_operand::apply_mach_visitor(expressions::mach_expr_visitor&) const {}

macro_operand_string::macro_operand_string(std::string value, const range operand_range)
    : macro_operand(mac_kind::STRING, operand_range)
    , value(std::move(value))
{}

void macro_operand_string::apply(operand_visitor& visitor) const { visitor.visit(*this); }

macro_operand_chain* macro_operand::access_chain()
{
    return kind == mac_kind::CHAIN ? static_cast<macro_operand_chain*>(this) : nullptr;
}

macro_operand_string* macro_operand::access_string()
{
    return kind == mac_kind::STRING ? static_cast<macro_operand_string*>(this) : nullptr;
}

macro_operand::macro_operand(mac_kind kind, range operand_range)
    : operand(operand_type::MAC, std::move(operand_range))
    , kind(kind)
{}


join_operands_result join_operands(const operand_list& operands)
{
    if (operands.empty())
        return {};

    join_operands_result result;
    size_t string_size = operands.size();

    for (const auto& op : operands)
        if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(op.get()))
            string_size += m_op->value.size();

    result.text.reserve(string_size);

    for (const auto& op : operands)
    {
        if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(op.get()))
            result.text.append(m_op->value);
        else
            result.text.push_back(',');

        result.ranges.push_back(op->operand_range);
    }
    result.total_range = union_range(operands.front()->operand_range, operands.back()->operand_range);

    return result;
}

struct request_halfword_alignment final : public expressions::mach_expr_visitor
{
    // Inherited via mach_expr_visitor
    void visit(const expressions::mach_expr_constant&) override {}
    void visit(const expressions::mach_expr_data_attr&) override {}
    void visit(const expressions::mach_expr_data_attr_literal&) override {}
    void visit(const expressions::mach_expr_symbol&) override {}
    void visit(const expressions::mach_expr_location_counter&) override {}
    void visit(const expressions::mach_expr_default&) override {}
    void visit(const expressions::mach_expr_literal& expr) override { expr.referenced_by_reladdr(); }
};

void transform_reloc_imm_operands(semantics::operand_list& op_list, context::id_index instruction)
{
    if (instruction.empty())
        return;

    unsigned char mask = 0;
    decltype(mask) top_bit = 1 << (std::numeric_limits<decltype(mask)>::digits - 1);

    if (auto mnem_tmp = context::instruction::find_mnemonic_codes(instruction.to_string_view()))
        mask = mnem_tmp->reladdr_mask().mask();
    else
        mask = context::instruction::get_machine_instructions(instruction.to_string_view()).reladdr_mask().mask();

    for (const auto& operand : op_list)
    {
        bool eligible = mask & top_bit;
        top_bit >>= 1;

        if (!eligible)
            continue;

        if (auto* mach_op = operand->access_mach(); mach_op != nullptr && mach_op->kind == mach_kind::EXPR)
        {
            auto& mach_expr = mach_op->access_expr()->expression;

            request_halfword_alignment visitor;
            mach_expr->apply(visitor);

            auto range = mach_expr->get_range();
            mach_expr = std::make_unique<expressions::mach_expr_binary<expressions::rel_addr>>(
                std::make_unique<expressions::mach_expr_location_counter>(range), std::move(mach_expr), range);
        }
    }
}

} // namespace hlasm_plugin::parser_library::semantics
