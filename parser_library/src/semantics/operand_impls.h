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

#ifndef SEMANTICS_OPERAND_IMPLS_H
#define SEMANTICS_OPERAND_IMPLS_H

#include <utility>
#include <vector>

#include "checking/data_definition/data_definition_operand.h"
#include "checking/instr_operand.h"
#include "concatenation.h"
#include "expressions/conditional_assembly/ca_expression.h"
#include "expressions/data_definition.h"
#include "expressions/mach_expression.h"
#include "operand.h"

// the file contains structures representing operands in the operand field of statement

namespace hlasm_plugin::parser_library::semantics {

// structure for empty operands
struct empty_operand final : operand
{
    empty_operand(const range& operand_range);

    void apply(operand_visitor& visitor) const override;
};

// operand that contains variable symbol thus is 'model operand'
struct model_operand final : operand
{
    static constexpr operand_type type_id = operand_type::MODEL;
    model_operand(concat_chain chain, std::vector<size_t> line_limits, const range& operand_range);

    concat_chain chain;
    std::vector<size_t> line_limits;

    void apply(operand_visitor& visitor) const override;
};

// operands that can return value and have dependencies
struct evaluable_operand : operand
{
    evaluable_operand(const operand_type type, const range& operand_range);

    virtual bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const = 0;

    virtual bool has_error(context::dependency_solver& info) const = 0;

    virtual std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const = 0;

    virtual void apply_mach_visitor(expressions::mach_expr_visitor&) const = 0;
};

// machine instruction operand
struct machine_operand final : operand
{
    static constexpr operand_type type_id = operand_type::MACH;
    explicit machine_operand(const range&);
    machine_operand(expressions::mach_expr_ptr displacement,
        expressions::mach_expr_ptr first_par,
        expressions::mach_expr_ptr second_par,
        const range& r);

    expressions::mach_expr_ptr displacement;
    expressions::mach_expr_ptr first_par;
    expressions::mach_expr_ptr second_par;

    bool has_dependencies(context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const;

    bool has_error(context::dependency_solver& info) const;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const;

    [[nodiscard]] bool is_single_expression() const noexcept { return displacement && !first_par && !second_par; }
};

enum class asm_kind
{
    EXPR,
    BASE_END,
    COMPLEX,
    STRING,
    TEXT,
};
struct expr_assembler_operand;
struct using_instr_assembler_operand;
struct complex_assembler_operand;
struct string_assembler_operand;
struct text_assembler_operand;

// assembler instruction operand
struct assembler_operand : evaluable_operand
{
    static constexpr operand_type type_id = operand_type::ASM;
    assembler_operand(const asm_kind kind, const range& r);

    expr_assembler_operand* access_expr() noexcept;
    using_instr_assembler_operand* access_base_end() noexcept;
    complex_assembler_operand* access_complex() noexcept;
    string_assembler_operand* access_string() noexcept;
    text_assembler_operand* access_text() noexcept;

    const expr_assembler_operand* access_expr() const noexcept;
    const using_instr_assembler_operand* access_base_end() const noexcept;
    const complex_assembler_operand* access_complex() const noexcept;
    const string_assembler_operand* access_string() const noexcept;
    const text_assembler_operand* access_text() const noexcept;

    const asm_kind kind;
};


// assembler expression operand
struct expr_assembler_operand final : assembler_operand
{
    expressions::mach_expr_ptr expression;

public:
    expr_assembler_operand(expressions::mach_expr_ptr expression, const range& operand_range);

    std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const override;
    bool has_error(context::dependency_solver& info) const override;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const override;

private:
    std::unique_ptr<checking::operand> get_operand_value_inner(
        context::dependency_solver& info, bool can_have_ordsym, diagnostic_op_consumer& diags) const;
};



// USING instruction operand
struct using_instr_assembler_operand final : assembler_operand
{
    using_instr_assembler_operand(expressions::mach_expr_ptr base,
        expressions::mach_expr_ptr end,
        std::string base_text,
        std::string end_text,
        const range& operand_range);

    bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const override;

    bool has_error(context::dependency_solver& info) const override;

    std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    expressions::mach_expr_ptr base;
    expressions::mach_expr_ptr end;
    std::string base_text;
    std::string end_text;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const override;
};



// complex assembler operand (i.e. 'OVERRIDE(A,B,C)')
struct complex_assembler_operand final : assembler_operand
{
    struct component_value_t
    {
        component_value_t()
            : op_range(range())
        {}
        component_value_t(range op_range)
            : op_range(op_range)
        {}

        virtual std::unique_ptr<checking::asm_operand> create_operand() const = 0;
        virtual ~component_value_t() = default;

        range op_range;
    };

    struct int_value_t final : component_value_t
    {
        int_value_t(int value, range range)
            : component_value_t(range)
            , value(value)
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            return std::make_unique<checking::one_operand>(value, op_range);
        }
        int value;
    };
    struct string_value_t final : component_value_t
    {
        // string_value_t(std::string value) : value(std::move(value)) {}
        string_value_t(std::string value, range range)
            : component_value_t(range)
            , value(std::move(value))
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            return std::make_unique<checking::one_operand>(value, op_range);
        }
        std::string value;
    };
    struct composite_value_t final : component_value_t
    {
        composite_value_t(std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, range range)
            : component_value_t(range)
            , identifier(std::move(identifier))
            , values(std::move(values))
        {}
        std::unique_ptr<checking::asm_operand> create_operand() const override
        {
            std::vector<std::unique_ptr<checking::asm_operand>> ret;
            for (auto& val : values)
                ret.push_back(val->create_operand());
            return std::make_unique<checking::complex_operand>(identifier, std::move(ret));
        }

        std::string identifier;
        std::vector<std::unique_ptr<component_value_t>> values;
    };

    complex_assembler_operand(
        std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, const range& operand_range);

    bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const override;

    bool has_error(context::dependency_solver& info) const override;

    std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    composite_value_t value;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const override;
};



// assembler string operand
struct string_assembler_operand final : assembler_operand
{
    string_assembler_operand(std::string value, const range& operand_range);

    bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const override;

    bool has_error(context::dependency_solver& info) const override;

    std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    std::string value;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const override;
};

// assembler text operand
struct text_assembler_operand final : assembler_operand
{
private:
    std::string value_;
    context::id_index ord_like_;

public:
    text_assembler_operand(std::string string_value, context::id_index ord_like, const range& operand_range);

    std::unique_ptr<checking::operand> get_operand_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    bool has_dependencies(
        context::dependency_solver& info, std::vector<context::id_index>* missing_symbols) const override;
    bool has_error(context::dependency_solver& info) const override;

    void apply(operand_visitor& visitor) const override;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const override;

    [[nodiscard]] const auto& get_text() const noexcept { return value_; }
    [[nodiscard]] const auto& get_ord_like() const noexcept { return ord_like_; }
};

// data definition operand
struct data_def_operand : operand
{
    static constexpr operand_type type_id = operand_type::DAT;
    std::shared_ptr<const expressions::data_definition> value;

    context::dependency_collector get_length_dependencies(context::dependency_solver& info) const;

    context::dependency_collector get_dependencies(context::dependency_solver& info) const;

    void apply(operand_visitor& visitor) const final;

    void apply_mach_visitor(expressions::mach_expr_visitor&) const;

    long long evaluate_total_length(context::dependency_solver& info,
        checking::data_instr_type checking_rules,
        diagnostic_op_consumer& diags) const;

protected:
    data_def_operand(std::shared_ptr<const expressions::data_definition> dd_ptr, const range& operand_range);
};

struct data_def_operand_shared final : data_def_operand
{
    data_def_operand_shared(std::shared_ptr<const expressions::data_definition> dd_ptr, const range& operand_range);
};

struct data_def_operand_inline final : data_def_operand
{
    expressions::data_definition data_def;

    data_def_operand_inline(expressions::data_definition data_def, const range& operand_range);
};

enum class ca_kind
{
    VAR,
    EXPR,
    SEQ,
    BRANCH
};
struct var_ca_operand;
struct expr_ca_operand;
struct seq_ca_operand;
struct branch_ca_operand;

// coditional assembly instruction operand
struct ca_operand : operand
{
    static constexpr operand_type type_id = operand_type::CA;
    ca_operand(const ca_kind kind, const range& operand_range);

    var_ca_operand* access_var();
    const var_ca_operand* access_var() const;
    expr_ca_operand* access_expr();
    const expr_ca_operand* access_expr() const;
    seq_ca_operand* access_seq();
    const seq_ca_operand* access_seq() const;
    branch_ca_operand* access_branch();
    const branch_ca_operand* access_branch() const;

    virtual bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx) = 0;

    const ca_kind kind;
};

// CA variable symbol operand
struct var_ca_operand final : ca_operand
{
    var_ca_operand(vs_ptr variable_symbol, const range& operand_range);

    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx) override;

    vs_ptr variable_symbol;

    void apply(operand_visitor& visitor) const override;
};

// CA expression operand
struct expr_ca_operand final : ca_operand
{
    expr_ca_operand(expressions::ca_expr_ptr expression, const range& operand_range);


    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx) override;

    expressions::ca_expr_ptr expression;

    void apply(operand_visitor& visitor) const override;
};

// CA sequence symbol operand
struct seq_ca_operand final : ca_operand
{
    seq_ca_operand(seq_sym sequence_symbol, const range& operand_range);


    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx) override;

    seq_sym sequence_symbol;

    void apply(operand_visitor& visitor) const override;
};

// CA branching operand (i.e. (5).here)
struct branch_ca_operand final : ca_operand
{
    branch_ca_operand(seq_sym sequence_symbol, expressions::ca_expr_ptr expression, const range& operand_range);


    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const expressions::evaluation_context& eval_ctx) override;

    seq_sym sequence_symbol;
    expressions::ca_expr_ptr expression;

    void apply(operand_visitor& visitor) const override;
};

struct macro_operand final : operand
{
    static constexpr operand_type type_id = operand_type::MAC;
    macro_operand(concat_chain chain, range operand_range);

    concat_chain chain;

    void apply(operand_visitor& visitor) const override;
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
