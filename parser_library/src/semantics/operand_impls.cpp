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

#include "context/using.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expr_visitor.h"
#include "expressions/mach_operator.h"
#include "instructions/instruction.h"
#include "operand_visitor.h"

namespace hlasm_plugin::parser_library::semantics {


//***************** operand *********************

operand::operand(const operand_type type, const range& operand_range)
    : type(type)
    , operand_range(operand_range)
{}

empty_operand::empty_operand(const range& operand_range)
    : operand(operand_type::EMPTY, operand_range)
{}

void empty_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

model_operand::model_operand(concat_chain chain, std::vector<size_t> line_limits, const range& operand_range)
    : operand(operand_type::MODEL, operand_range)
    , chain(std::move(chain))
    , line_limits(std::move(line_limits))
{}

void model_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

evaluable_operand::evaluable_operand(const operand_type type, const range& operand_range)
    : operand(type, operand_range)
{}

//***************** machine_operand *********************

machine_operand::machine_operand(const range& r)
    : operand(operand_type::MACH, r)
{}

machine_operand::machine_operand(expressions::mach_expr_ptr displacement,
    expressions::mach_expr_ptr first_par,
    expressions::mach_expr_ptr second_par,
    const range& r)
    : operand(operand_type::MACH, r)
    , displacement(std::move(displacement))
    , first_par(std::move(first_par))
    , second_par(std::move(second_par))
{
    assert(this->displacement);
}

bool machine_operand::has_dependencies(
    context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const
{
    return displacement && displacement->has_dependencies(info, missing_symbols)
        || first_par && first_par->has_dependencies(info, missing_symbols)
        || second_par && second_par->has_dependencies(info, missing_symbols);
}

bool machine_operand::has_error(context::dependency_solver& info) const
{
    return displacement && displacement->get_dependencies(info).has_error
        || first_par && first_par->get_dependencies(info).has_error
        || second_par && second_par->get_dependencies(info).has_error;
}

void machine_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

void machine_operand::apply_mach_visitor(expressions::mach_expr_visitor& v) const
{
    if (displacement)
        displacement->apply(v);
    if (first_par)
        first_par->apply(v);
    if (second_par)
        second_par->apply(v);
}

assembler_operand::assembler_operand(const asm_kind kind, const range& r)
    : evaluable_operand(operand_type::ASM, r)
    , kind(kind)
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

const expr_assembler_operand* assembler_operand::access_expr() const
{
    return kind == asm_kind::EXPR ? static_cast<const expr_assembler_operand*>(this) : nullptr;
}

const using_instr_assembler_operand* assembler_operand::access_base_end() const
{
    return kind == asm_kind::BASE_END ? static_cast<const using_instr_assembler_operand*>(this) : nullptr;
}

const complex_assembler_operand* assembler_operand::access_complex() const
{
    return kind == asm_kind::COMPLEX ? static_cast<const complex_assembler_operand*>(this) : nullptr;
}

const string_assembler_operand* assembler_operand::access_string() const
{
    return kind == asm_kind::STRING ? static_cast<const string_assembler_operand*>(this) : nullptr;
}

//***************** expr_assembler_operand *********************

expr_assembler_operand::expr_assembler_operand(
    expressions::mach_expr_ptr expression, std::string string_value, const range& operand_range)
    : assembler_operand(asm_kind::EXPR, operand_range)
    , expression(std::move(expression))
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
    return expression->has_dependencies(info, missing_symbols);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_assembler_operand::has_error(context::dependency_solver& info) const
{
    return expression->get_dependencies(info).has_error;
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
    const range& operand_range)
    : assembler_operand(asm_kind::BASE_END, operand_range)
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
    std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, const range& operand_range)
    : assembler_operand(asm_kind::COMPLEX, operand_range)
    , value(std::move(identifier), std::move(values), operand_range)
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
ca_operand::ca_operand(const ca_kind kind, const range& operand_range)
    : operand(operand_type::CA, operand_range)
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

var_ca_operand::var_ca_operand(vs_ptr variable_symbol, const range& operand_range)
    : ca_operand(ca_kind::VAR, operand_range)
    , variable_symbol(std::move(variable_symbol))
{}

bool var_ca_operand::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expressions::ca_var_sym::get_undefined_attributed_symbols_vs(symbols, variable_symbol, eval_ctx);
}

void var_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

expr_ca_operand::expr_ca_operand(expressions::ca_expr_ptr expression, const range& operand_range)
    : ca_operand(ca_kind::EXPR, operand_range)
    , expression(std::move(expression))
{}

bool expr_ca_operand::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(symbols, eval_ctx);
}

void expr_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

seq_ca_operand::seq_ca_operand(seq_sym sequence_symbol, const range& operand_range)
    : ca_operand(ca_kind::SEQ, operand_range)
    , sequence_symbol(std::move(sequence_symbol))
{}

bool seq_ca_operand::get_undefined_attributed_symbols(
    std::vector<context::id_index>&, const expressions::evaluation_context&)
{
    return false;
}

void seq_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

branch_ca_operand::branch_ca_operand(
    seq_sym sequence_symbol, expressions::ca_expr_ptr expression, const range& operand_range)
    : ca_operand(ca_kind::BRANCH, operand_range)
    , sequence_symbol(std::move(sequence_symbol))
    , expression(std::move(expression))
{}

bool branch_ca_operand::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(symbols, eval_ctx);
}

void branch_ca_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

macro_operand::macro_operand(concat_chain chain, range operand_range)
    : operand(operand_type::MAC, operand_range)
    , chain(std::move(chain))
{}

void macro_operand::apply(operand_visitor& visitor) const { visitor.visit(*this); }

data_def_operand::data_def_operand(
    std::shared_ptr<const expressions::data_definition> dd_ptr, const range& operand_range)
    : operand(operand_type::DAT, operand_range)
    , value(std::move(dd_ptr))
{}

data_def_operand_inline::data_def_operand_inline(expressions::data_definition val, const range& operand_range)
    : data_def_operand(
          std::shared_ptr<const expressions::data_definition>(std::shared_ptr<const void>(), &data_def), operand_range)
    , data_def(std::move(val))
{}

data_def_operand_shared::data_def_operand_shared(
    std::shared_ptr<const expressions::data_definition> dd_ptr, const range& operand_range)
    : data_def_operand(std::move(dd_ptr), operand_range)
{}

context::dependency_collector data_def_operand::get_length_dependencies(context::dependency_solver& info) const
{
    return value->get_length_dependencies(info);
}

context::dependency_collector data_def_operand::get_dependencies(context::dependency_solver& info) const
{
    return value->get_dependencies(info);
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

checking::data_definition_operand data_def_operand::get_operand_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    checking::data_definition_operand op;

    op.operand_range = operand_range;

    op.dupl_factor = value->evaluate_dupl_factor(info, diags);
    op.type.value = value->type;
    op.type.rng = value->type_range;
    op.extension.present = value->extension != '\0';
    op.extension.value = value->extension;
    op.extension.rng = value->extension_range;
    op.length = value->evaluate_length(info, diags);
    op.scale = value->evaluate_scale(info, diags);
    op.exponent = value->evaluate_exponent(info, diags);

    op.nominal_value = value->evaluate_nominal_value(info, diags);

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

string_assembler_operand::string_assembler_operand(std::string value, const range& operand_range)
    : assembler_operand(asm_kind::STRING, operand_range)
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

void transform_reloc_imm_operands(semantics::operand_list& op_list, const processing::op_code& op)
{
    unsigned char mask = 0;

    switch (op.type)
    {
        case context::instruction_type::MACH:
            mask = (unsigned char)op.instr_mach->reladdr_mask();
            break;
        case context::instruction_type::MNEMO:
            mask = (unsigned char)op.instr_mnemo->reladdr_mask();
            break;
        default:
            return;
    }

    decltype(mask) top_bit = 1 << (std::numeric_limits<decltype(mask)>::digits - 1);

    for (const auto& operand : op_list)
    {
        bool eligible = mask & top_bit;
        top_bit >>= 1;

        if (!eligible)
            continue;

        if (auto* mach_op = operand->access_mach(); mach_op != nullptr && mach_op->is_single_expression())
        {
            auto& mach_expr = mach_op->displacement;

            request_halfword_alignment visitor;
            mach_expr->apply(visitor);

            auto range = mach_expr->get_range();
            mach_expr = std::make_unique<expressions::mach_expr_binary<expressions::rel_addr>>(
                std::make_unique<expressions::mach_expr_location_counter>(range), std::move(mach_expr), range);
        }
    }
}
} // namespace hlasm_plugin::parser_library::semantics
